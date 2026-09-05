# Security Architecture, Middleware & Threat Defense

---

## 1. Executive Security Summary & Zero-Trust Principles

**CrowApi** is architected on a strict **Zero-Trust Security Model**: *never trust, always verify*. Every incoming HTTP request must prove its authenticity, integrity, and authorization on every endpoint. No assumption of perimeter security or stateful cookies is made.

### Core Security Guarantees
* **Asymmetric RS256 Cryptography**: All JWT access tokens are signed with 2048-bit RSA private keys; public keys are discoverable via standard RFC 7517 JWKS.
* **RFC 7636 PKCE S256**: All Google OAuth flows enforce Proof Key for Code Exchange using cryptographically random verifiers and SHA-256 challenges, verified against official RFC test vectors.
* **Refresh Token Family Revocation**: Refresh tokens rotate on every invocation. If a previously rotated token is presented again (replay attack), the entire family of sessions is immediately revoked.
* **Real-Time JTI Blacklisting**: Instantaneous token revocation across all cluster nodes using PostgreSQL `LISTEN / NOTIFY` on channel `session_revoked`.
* **Zero SQL Injection Surface**: 100% of database queries execute through `pqxx::params` prepared statements.
* **Brute-Force Lockout Defense**: 5 consecutive failed login attempts lock the target account for 15 minutes.

---

## 2. Defense-in-Depth Architecture

The system implements multiple overlapping layers of defense so that if one security control is compromised, subsequent layers prevent exploitation:

```mermaid
flowchart TD
    subgraph Layer1 ["Layer 1: Edge & Network Security"]
        Req["Incoming TLS HTTP Request"] --> TLS["TLS 1.3 Termination<br/>Strict Transport Security (HSTS)"]
        TLS --> RateLimit["IP & Ingress Rate Limiting"]
    end

    subgraph Layer2 ["Layer 2: Transport & Middleware Pipeline"]
        RateLimit --> LogMware["LoggingMiddleware<br/>Microsecond Request Tracing & Client IP"]
        LogMware --> AuthCtx["HttpResponseHelper::extractAuthenticatedUser<br/>Bearer Token Parsing"]
    end

    subgraph Layer3 ["Layer 3: Cryptographic Verification"]
        AuthCtx --> AlgCheck["Algorithm Enforcement<br/>Must be RS256 (Block alg: none)"]
        AlgCheck --> KidLookup["Key Manager JWKS Lookup<br/>Retrieve RSA Public Key by kid"]
        KidLookup --> SigVerify["OpenSSL 3.x EVP_DigestVerify<br/>RS256 Signature Check"]
        SigVerify --> TimeClaims["Claims Validation<br/>exp > now, nbf <= now, iss, aud"]
    end

    subgraph Layer4 ["Layer 4: State & Revocation Check"]
        TimeClaims --> FastJtiCheck["Fast Blacklist Query<br/>SELECT 1 FROM token_revocations WHERE jti = $1"]
        FastJtiCheck --> SessActive["Session Liveness Check<br/>revoked_at IS NULL AND expires_at > now"]
        SessActive --> UserActive["User Status Check<br/>is_active == true AND locked_until IS NULL"]
    end

    subgraph Layer5 ["Layer 5: Business Authorization & RBAC"]
        UserActive --> RBAC["AuthorizationService<br/>Role & Permission Checks"]
        RBAC --> IDOR["Resource Ownership Check<br/>userId == resource.ownerId"]
        IDOR --> Controller["Target Controller Handler"]
    end
```

---

## 3. Threat Mitigation Matrix

| Vulnerability / Attack Vector | Severity | Mitigating Security Control in CrowApi | Verification Mechanism |
| :--- | :--- | :--- | :--- |
| **SQL Injection (SQLi / CWE-89)** | Critical | 100% parameterized SQL via `pqxx::params`. Zero string concatenation. Input sanitization in validators. | Tested in `Security::CyberAttacks/SqlInjection*` |
| **OAuth Code Interception (RFC 7636)** | High | Mandatory PKCE with `S256` method. 64-char CSPRNG verifier and SHA-256 base64url challenge. | Tested in `Security::PKCE/*` |
| **State Replay Attack (OAuth2)** | High | Single-use CSRF `state` and `code_verifier` cached in `cache_store` with 600s TTL, immediately deleted on callback. | Tested in `Application::UseCases/GoogleLogin*` |
| **JWT Signature Tampering** | Critical | OpenSSL 3.x RSA 2048-bit digital signature. Modifying 1 bit invalidates verification. | Tested in `Security::CyberAttacks/JwtSignatureTamperingRejected` |
| **Algorithm Confusion (alg: none / CWE-327)** | Critical | `JwtService` hardcodes check `header.alg == "RS256"`. Any other algorithm is rejected immediately. | Tested in `Security::CyberAttacks/JwtAlgNoneVulnerabilityBlocked` |
| **Key Injection / Unknown `kid`** | High | `RsaKeyManager` only validates against keys present in its internal key registry. Unknown `kid` fails immediately. | Tested in `Security::CyberAttacks/JwtUnknownKeyIdRejected` |
| **Revoked Token Replay** | High | `token_revocations` table and in-memory blacklist reject revoked JTIs with 401 Unauthorized. | Tested in `Security::CyberAttacks/BannedJtiTokenBlockedImmediately` |
| **Refresh Token Theft & Reuse** | High | Token rotation stores SHA-256 hash. Presenting `previous_refresh_token_hash` revokes entire session family. | Tested in `Security::CyberAttacks/RefreshTokenReplayDetected*` |
| **Insecure Direct Object References (IDOR)** | Medium | `AuthorizationService` and controllers strictly compare resource owner UUID against token's `sub`. | Tested in `Security::CyberAttacks/IdorAuthorizationBypassBlocked` |
| **Credential Stuffing / Brute-Force** | Medium | Failed attempt counter. 5 consecutive failures triggers 15-minute account lock (`locked_until`). | Tested in `Security::CyberAttacks/BruteForceTriggersAccountLockout` |
| **Cross-Site Scripting (XSS / CWE-79)** | Medium | Pure JSON API. No HTML rendering. String inputs treated as literal data. | Tested in `Security::CyberAttacks/XssPayloadsHandledAsLiteralStrings` |
| **Buffer Overrun / Corrupt Input** | High | Modern C++20 `std::string`, `std::string_view`, bounds-checked base64 decoding. | Tested in `Security::CyberAttacks/MalformedBase64DoesNotCrash` |
| **PKCE Secrets Disclosure (CWE-200)** | High | Server-side secret encapsulation: `codeVerifier`, `codeChallenge`, and `state` omitted from HTTP JSON responses. | Tested in `Security::CyberAttacks/PkceSecretsNotExposedInHttpResponse` |
| **Forged PKCE State Callback (CSRF)** | High | Strict cache validation on callback: unmapped, missing, or forged states rejected with 401 Unauthorized before token exchange. | Tested in `Security::CyberAttacks/ForgedPkceStateRejectedOnCallback` |

---

## 4. Security Middleware & Authentication Pipeline

Protected endpoints do not read credentials directly. Instead, they execute through the centralized security middleware pipeline implemented in `HttpResponseHelper::extractAuthenticatedUser`:

```mermaid
sequenceDiagram
    autonumber
    actor Client
    participant HTTP as Crow ASIO Server
    participant Helper as HttpResponseHelper
    participant JWT as JwtService (OpenSSL 3.x)
    participant SessRepo as PostgresSessionRepository
    participant UserRepo as PostgresUserRepository
    participant Controller as Presentation Controller

    Client->>HTTP: GET /api/v1/sessions<br/>Authorization: Bearer eyJhbGciOiJSUzI1Ni...
    HTTP->>Controller: Route Match
    Controller->>Helper: extractAuthenticatedUser(req, jwtService, sessionRepo, userRepo)

    Note over Helper: Step 1: Header Validation
    Helper->>Helper: Check "Authorization: Bearer <token>" format
    alt Header Missing or Malformed
        Helper-->>Controller: Result::err(401 Unauthorized, "Missing or invalid Bearer header")
        Controller-->>Client: 401 Unauthorized
    end

    Note over Helper: Step 2: Cryptographic Signature & Claims Check
    Helper->>JWT: validateAccessToken(token)
    JWT->>JWT: 1. Parse Header: ensure alg == "RS256", extract kid<br/>2. Retrieve RSA Public Key for kid<br/>3. EVP_DigestVerify signature over header.payload<br/>4. Validate: exp > now, nbf <= now, iss, aud
    alt Signature Invalid, Expired, or Bad Claims
        JWT-->>Helper: Result::err(401 Unauthorized, errorDetails)
        Helper-->>Controller: Result::err(...)
        Controller-->>Client: 401 Unauthorized
    end
    JWT-->>Helper: Valid JwtClaims (sub: userId, sid: sessionId, jti, role)

    Note over Helper: Step 3: Fast JTI Revocation Blacklist Check
    Helper->>SessRepo: isTokenRevoked(claims.jti)
    alt JTI Found in token_revocations
        SessRepo-->>Helper: true (Token Revoked)
        Helper-->>Controller: Result::err(401 Unauthorized, "Access token has been revoked")
        Controller-->>Client: 401 Unauthorized
    end

    Note over Helper: Step 4: Parent Session Liveness Check
    Helper->>SessRepo: findById(claims.sid)
    alt Session Not Found, Revoked, or Expired
        SessRepo-->>Helper: nullopt or session.isRevoked()
        Helper-->>Controller: Result::err(401 Unauthorized, "Session has been revoked or expired")
        Controller-->>Client: 401 Unauthorized
    end

    Note over Helper: Step 5: User Account Status Check
    Helper->>UserRepo: findById(claims.sub)
    alt User Inactive or Locked
        UserRepo-->>Helper: user.isActive == false
        Helper-->>Controller: Result::err(401 Unauthorized, "User account is inactive or disabled")
        Controller-->>Client: 401 Unauthorized
    end

    Note over Helper: Step 6: AuthenticatedUser Context Created
    Helper-->>Controller: Result::ok(AuthenticatedUser { id, email, role, sessionId, jti })
    Controller->>Controller: Execute Controller Business Logic
    Controller-->>Client: 200 OK + JSON Response Envelope
```

---

## 5. JWT Architecture & Cryptographic Lifecycle

### 5.1 Token Anatomy (RS256)
Every issued access token consists of three Base64URL-encoded segments:

```text
eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImtleS0yMDI2LXByb2QtMDEifQ
.
eyJpc3MiOiJjcm93LWFwaS1hdXRoIiwiYXVkIjoiY3J3LWFwaS1jbGllbnRzIiwic3ViIjoiZWJmZWU5MWEtYTQ0OC00YWE4LTgwYjctMDIyZTU2NmM3MDVkIiwic2lkIjoiOTdjM2YzNGYtYzk4Zi00MTE1LTkzZTctNmVmYTM0OWY5YzQ0IiwianRpIjoiY2FhYjc0YTQtNjMwYi00NjA2LWE2MGMtYjY1ZDE5ZDhiYTM0Iiwicm9sZSI6InVzZXIiLCJpYXQiOjE3ODg1OTE0ODQsImV4cCI6MTc4ODU5MjM4NCwibmJmIjoxNzg4NTkxNDg0fQ
.
DLZsXYfvOh2ZRB81615FHDxzxKicpMQ5Llmo8oN1stLaucpPrOlVVOVgOiUOT...
```

1. **Header**:
   * `alg`: Strictly `"RS256"` (RSA Signature with SHA-256).
   * `typ`: `"JWT"`.
   * `kid`: Key ID referencing the specific signing key in the JWKS (e.g. `"key-2026-prod-01"`).
2. **Payload (Claims)**:
   * `iss`: Issuer (`"crow-api-auth"`).
   * `aud`: Audience (`"crow-api-clients"`).
   * `sub`: User ID (UUID string).
   * `sid`: Associated Session ID in PostgreSQL `sessions` table (UUID string).
   * `jti`: Unique JWT ID for tracking and instant blacklisting (UUID string).
   * `role`: User role (`"user"`, `"admin"`, `"superadmin"`).
   * `iat`: Issued At timestamp (Unix epoch seconds).
   * `exp`: Expiration timestamp (15 minutes from issue).
   * `nbf`: Not Before timestamp (Unix epoch seconds).
3. **Signature**:
   * Cryptographic signature computed via OpenSSL 3.x `EVP_DigestSign` using the 2048-bit RSA private key over `ASCII(Header) || '.' || ASCII(Payload)`.

### 5.2 Seamless RSA Key Rotation (Zero Downtime)
Key compromise or scheduled rotation does not invalidate active user sessions:

```mermaid
flowchart LR
    subgraph KeyManager ["RsaKeyManager Registry"]
        ActiveKey["Active Key: kid = 'key-2026-prod-02'<br/>(Signs all new JWTs)"]
        RetiredKey["Retired Key: kid = 'key-2026-prod-01'<br/>(Verifies existing valid tokens)"]
    end

    subgraph Discovery ["Public Discovery Endpoint"]
        JWKS["GET /.well-known/jwks.json<br/>Exposes Public Keys for BOTH kid-01 and kid-02"]
    end

    subgraph Verification ["Token Verification Engine"]
        IncomingToken["Incoming Request Token<br/>Header kid = 'key-2026-prod-01'"]
        IncomingToken --> Match["Lookup kid in Registry"]
        Match --> RetiredKey
        RetiredKey --> ValidSig["Signature Verified Successfully!"]
    end

    KeyManager --> JWKS
```

---

## 6. Multi-Session Management & Refresh Token Security

### 6.1 Database Session Storage & Cryptographic Hashing
Plain refresh tokens are **never stored in the database**. The system only stores SHA-256 cryptographic hashes:

```mermaid
flowchart TD
    ClientToken["Client Holds Raw Token:<br/>'0XqNFcGfVV1IkmkWxcbJnedLIQmceJc3zTPXOwgMTIc'"]
    ClientToken --> Hash["OpenSSL SHA-256 Digest"]
    Hash --> StoredHash["Stored in PostgreSQL sessions table:<br/>refresh_token_hash: 'a3f89e21b...'"]
```

If the database is leaked or dumped, an attacker **cannot authenticate** because SHA-256 is mathematically irreversible.

### 6.2 Refresh Token Rotation State Machine
Every token refresh operation cycles the tokens:

```mermaid
stateDiagram-v2
    [*] --> ActiveSession: Login / Registration

    state ActiveSession {
        [*] --> TokenPairValid: Issued (Access JWT + Refresh Token 1)
        TokenPairValid --> RotatedPair: POST /api/v1/auth/refresh with Refresh Token 1
        note right of RotatedPair
            refresh_token_hash = Hash(Token 2)
            previous_refresh_token_hash = Hash(Token 1)
        end note
    }

    RotatedPair --> TokenReplayDetected: Attacker submits compromised Token 1 again!
    
    state TokenReplayDetected {
        [*] --> AlertLogged: Log SECURITY_ALERT_TOKEN_REPLAY
        AlertLogged --> RevokeAll: Revoke ALL sessions for user!
        RevokeAll --> BlacklistJtis: Write JTIs to token_revocations
    }

    RotatedPair --> ActiveSession: Normal refresh cycle repeats
    ActiveSession --> TerminatedSession: User Logout / Admin Revoke
    TerminatedSession --> [*]
```

---

## 7. Cross-Component Interactions: DB, Cache & Real-Time Events

### 7.1 PostgreSQL `LISTEN / NOTIFY` Distributed Revocation Architecture

In distributed or clustered deployments, session revocation on Node A must instantly invalidate tokens on Node B without hitting the database on every subsequent request:

```mermaid
sequenceDiagram
    autonumber
    actor Admin
    participant NodeA as CrowApi Node A
    participant DB as PostgreSQL 18.6
    participant ListenerB as Background jthread (Node B)
    participant MemoryCacheB as In-Memory JTI Blacklist (Node B)
    participant ClientUser as Client User (Node B)

    Admin->>NodeA: DELETE /api/v1/admin/sessions/{sessionId}
    NodeA->>DB: 1. UPDATE sessions SET revoked_at = now() WHERE id = $1<br/>2. INSERT INTO token_revocations (jti, session_id, expires_at)<br/>3. SELECT pg_notify('session_revoked', '{sessionId}')
    
    DB-->>ListenerB: Real-Time Async Notification: channel='session_revoked', payload='{sessionId}'
    ListenerB->>MemoryCacheB: Insert JTI / Session ID into In-Memory Blacklist Cache
    
    Note over ClientUser,NodeB: 5 milliseconds later on Node B...
    ClientUser->>NodeB: GET /api/v1/todos (using revoked session JWT)
    NodeB->>MemoryCacheB: Check JTI against In-Memory Blacklist
    MemoryCacheB-->>NodeB: Match Found (BANNED)
    NodeB-->>ClientUser: 401 Unauthorized { message: "Token has been revoked" }
```

---

---

## 8. RFC 7636 PKCE Security Architecture & Privacy Hardening

### 8.1 The Security Dilemma: Authorization Code Interception & Secrets Leakage
In standard OAuth 2.0 authorization code flows, the authorization server returns an authorization code via a browser redirect. On mobile devices, SPAs, and distributed environments, this code is vulnerable to interception (e.g., via malicious custom URI scheme handlers, rogue browser extensions, or network proxy logs).

RFC 7636 (Proof Key for Code Exchange) eliminates this vulnerability by introducing a dynamic cryptographic secret (`code_verifier`) and its one-way hashed counterpart (`code_challenge`). However, naive implementations leak the secret back to the frontend client in JSON API responses, violating **CWE-200 (Exposure of Sensitive Information to an Unauthorized Actor)**.

CrowApi enforces **Complete Server-Side Secret Encapsulation**:

```mermaid
flowchart TD
    subgraph ClientPublic ["Public Domain (Browser / Mobile Client)"]
        Resp["HTTP Response: 200 OK<br/>data: { authUrl: 'https://accounts.google.com/...' }<br/><b>codeVerifier, codeChallenge, state OMITTED</b>"]
        BrowserNav["Browser Navigates to authUrl<br/>Query String: client_id, redirect_uri, <b>state</b>, <b>code_challenge</b>, S256"]
        Resp --> BrowserNav
    end

    subgraph ServerPrivate ["Private Domain (CrowApi Backend & PostgreSQL)"]
        Gen["OpenSSL CSPRNG<br/>1. code_verifier (64-char unreserved)<br/>2. state (128-bit nonce)"]
        Hash["Compute One-Way Hash<br/>code_challenge = BASE64URL(SHA-256(verifier))"]
        Store["Store in PostgreSQL cache_store (TTL: 600s)<br/>Key: 'pkce:state:' + state<br/>Value: code_verifier<br/><b>Isolated in memory/DB</b>"]
        Gen --> Hash --> Store
    end

    subgraph GoogleIdP ["Google Identity Services"]
        GoogleAuth["Google Authorization Server<br/>Receives: code_challenge & state<br/>Saves challenge fingerprint"]
        GoogleToken["Google Token Endpoint<br/>POST /token<br/>Receives: code & code_verifier<br/>Validates: SHA256(verifier) == challenge"]
    end

    Store -.->|Embedded in authUrl| BrowserNav
    BrowserNav -->|User Grants Consent| GoogleAuth
    GoogleAuth -->|302 Redirect with code & state| Callback["CrowApi GET /api/v1/auth/google/callback"]
    Callback -->|Evict verifier from cache| TokenReq["Direct Outbound TLS Back-Channel"]
    TokenReq -->|Sends code_verifier| GoogleToken
    GoogleToken -->|Issue ID & Access Tokens| Success["Authenticated Session Established"]
```

---

### 8.2 Parameter Anatomy: Why Each Parameter Exists and How Opacity is Guaranteed

| Parameter | Location | Is it Secret? | Why It Must Exist | How Opacity & Integrity Are Guaranteed |
| :--- | :--- | :---: | :--- | :--- |
| **`code_verifier`** | Kept in PostgreSQL `cache_store` only. Never in URL, never in JSON. | **YES (Master Secret)** | Required by Google's `/token` endpoint during server-to-server exchange to prove the entity requesting tokens is the one who initiated the flow. | **Zero Transmission**: Generated via OpenSSL `RAND_bytes`. Never sent to the client, never logged, and immediately deleted from PostgreSQL on callback. |
| **`code_challenge`** | Query parameter inside `authUrl`. Omitted from JSON body. | **NO (Public Commitment)** | Mandatory for Google’s authorization endpoint (`https://accounts.google.com/o/oauth2/v2/auth`). Google must record this hash so it can verify the verifier later. | **One-Way SHA-256 Hash**: Because SHA-256 is mathematically irreversible, extracting `code_verifier` from `code_challenge` is computationally infeasible ($2^{256}$ operations). |
| **`state`** | Query parameter inside `authUrl` and returned in Google callback. Omitted from JSON body. | **NO (Public Nonce)** | Mandatory OAuth 2.0 (RFC 6749 §10.12) protection against Cross-Site Request Forgery (CSRF). Binds browser callback to session. | **128-bit Cryptographic Nonce**: Generated via OpenSSL CSPRNG. Contains zero user identifiers, zero account info, and zero system timestamps. Acts as an opaque, single-use lookup token. |

---

### 8.3 Cyber-Attack Penetration Tests for PKCE

CrowApi's test suite includes automated penetration tests in `Tests/Security/CyberAttacksTest.cpp` to prove that PKCE privacy and anti-forgery mechanisms cannot be bypassed:

#### 1. `PkceSecretsNotExposedInHttpResponse`
* **Threat Guarded Against**: **CWE-200 / CWE-598 (Information Disclosure)**. An attacker eavesdropping on client API traffic, inspecting client memory, or viewing client logs attempts to obtain the raw `codeVerifier`, `codeChallenge`, or `state`.
* **Penetration Scenario**: The test executes `HttpResponseHelper::serializeGoogleAuthUrlDto` with populated DTO fields.
* **Verification Invariant**: Asserts that substrings `"codeVerifier"`, `"codeChallenge"`, and the raw verifier string are completely absent from the serialized JSON string. Only `"authUrl"` is returned.
* **Architectural Importance**: Prevents downstream clients (mobile apps, web SPAs) from accidentally logging or leaking cryptographic secrets.

#### 2. `ForgedPkceStateRejectedOnCallback`
* **Threat Guarded Against**: **Cross-Site Request Forgery (CSRF / CWE-352) & Code Injection**. An attacker intercepts an authorization code from an arbitrary user or crafts a malicious link directing an unsuspecting user to `/api/v1/auth/google/callback` with a forged `state`.
* **Penetration Scenario**: Attacker invokes `loginWithGoogle` with `state = "attacker-forged-state-xyz"` that was never registered by the server.
* **Verification Invariant**: Asserts that `loginWithGoogle` immediately fails with `Result::err`, returning HTTP **`401 Unauthorized`**. No request is dispatched to Google, and no session is created.
* **Architectural Importance**: Guarantees that only callbacks originating from an authentic, server-initiated authorization flow can ever initiate a token exchange.

---

## 9. Summary of Hardened Security Defenses

1. **RFC 7636 PKCE S256**: All Google OAuth flows enforce Proof Key for Code Exchange using cryptographically random verifiers and SHA-256 challenges, with complete server-side encapsulation.
2. **Replay Attack Defenses**: Refresh tokens use hash rotation (`refresh_token_hash` and `previous_refresh_token_hash`). If an expired or already rotated token is replayed, the entire session family is automatically revoked.
3. **Database Fault Recovery**: If the PostgreSQL container restarts, the connection pool detects connection failures, flushes stale file descriptors, and reconnects without crashing the web service.
4. **Rate Limiting & Account Protection**: 5 consecutive failed login attempts automatically lock the user account for 15 minutes (`locked_until`), guarding against brute-force password discovery.
5. **No plain tokens on disk**: Passwords hashed with PBKDF2 (100k rounds, 16-byte salt), refresh tokens hashed with SHA-256.
6. **Strict ISO C++20 Memory Safety**: Zero manual `new`/`delete`; all lifetimes managed by RAII (`std::shared_ptr`, `std::unique_ptr`, `pqxx::work`, `EVP_PKEY_free`).
7. **Comprehensive Penetration Testing**: 13 automated cyber-attack tests asserting resilience against SQLi, XSS, JWT tampering, algorithm confusion, key injection, token replay, IDOR, brute-force, buffer overflows, PKCE secret exposure, and forged CSRF states.
