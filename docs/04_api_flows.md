# 04. API Flows, OAuth2 PKCE & Security Sequences

---

## 1. Authentication & Security Sequences

### 1.1 Standard User Registration Flow (`POST /api/v1/auth/register`)

```mermaid
sequenceDiagram
    autonumber
    actor Client
    participant AuthCtrl as Presentation::AuthController
    participant Validator as Application::AuthInputValidator
    participant AuthUC as Application::AuthUseCases
    participant Hasher as Infrastructure::OpenSslCrypto
    participant UserRepo as Infrastructure::PostgresUserRepository
    participant Audit as Infrastructure::PostgresAuditLogRepository

    Client->>AuthCtrl: POST /api/v1/auth/register { email, password }
    AuthCtrl->>Validator: validateRegisterInput(email, password)
    alt Validation Failed (Malformed email or weak password)
        Validator-->>AuthCtrl: ValidationError (INVALID_INPUT)
        AuthCtrl-->>Client: 400 Bad Request { success: false, errors: [...] }
    else Validation Passed
        Validator-->>AuthCtrl: OK
        AuthCtrl->>AuthUC: register(RegisterRequest)
        AuthUC->>UserRepo: findByEmail(email)
        alt Email Already Exists
            UserRepo-->>AuthUC: User entity found
            AuthUC-->>AuthCtrl: Result::failure(CONFLICT)
            AuthCtrl-->>Client: 409 Conflict { message: "Email already registered" }
        else Email Available
            AuthUC->>Hasher: hashPassword(password) [PBKDF2-HMAC-SHA256, 100k iterations]
            Hasher-->>AuthUC: "pbkdf2:sha256:100000:<salt>:<hash>"
            AuthUC->>UserRepo: save(newUser)
            UserRepo-->>AuthUC: Saved User Entity
            AuthUC->>Audit: recordEvent("USER_REGISTERED", userId)
            AuthUC-->>AuthCtrl: Result::success(UserResponse)
            AuthCtrl-->>Client: 201 Created { success: true, data: { id, email, role } }
        end
    end
```

---

### 1.2 Local User Login & Brute-Force Defenses (`POST /api/v1/auth/login`)

```mermaid
sequenceDiagram
    autonumber
    actor Client
    participant AuthCtrl as Presentation::AuthController
    participant AuthUC as Application::AuthUseCases
    participant Hasher as Infrastructure::OpenSslCrypto
    participant Jwt as Infrastructure::JwtService
    participant SessionRepo as Infrastructure::PostgresSessionRepository
    participant UserRepo as Infrastructure::PostgresUserRepository

    Client->>AuthCtrl: POST /api/v1/auth/login { email, password, deviceName }
    AuthCtrl->>AuthUC: login(LoginRequest)
    AuthUC->>UserRepo: findByEmail(email)
    alt User Not Found
        UserRepo-->>AuthUC: nullopt
        AuthUC-->>AuthCtrl: Result::failure(INVALID_CREDENTIALS)
        AuthCtrl-->>Client: 401 Unauthorized { message: "Invalid credentials" }
    else User Exists
        alt Account Locked (failed_login_attempts >= 5 AND locked_until > now)
            AuthUC-->>AuthCtrl: Result::failure(ACCOUNT_LOCKED)
            AuthCtrl-->>Client: 423 Locked { message: "Account locked. Try again later." }
        else Account Normal
            AuthUC->>Hasher: verifyPassword(password, user.password_hash)
            alt Password Invalid
                Hasher-->>AuthUC: false
                AuthUC->>UserRepo: incrementFailedLoginAttempts(userId)
                Note over AuthUC,UserRepo: If attempts >= 5, set locked_until = now() + 15 mins
                AuthUC-->>AuthCtrl: Result::failure(INVALID_CREDENTIALS)
                AuthCtrl-->>Client: 401 Unauthorized
            else Password Correct
                Hasher-->>AuthUC: true
                AuthUC->>UserRepo: resetFailedLoginAttempts(userId)
                AuthUC->>SessionRepo: createSession(newSession)
                AuthUC->>Jwt: generateAccessToken(user, session) [RS256 Private Key]
                Jwt-->>AuthUC: JWT String (kid: key-2026-prod-01)
                AuthUC-->>AuthCtrl: Result::success(AuthResponse)
                AuthCtrl-->>Client: 200 OK { accessToken, refreshToken, expiresIn: 900, user }
            end
        end
    end
```

---

### 1.3 Google OAuth 2.0 / OIDC Flow with RFC 7636 PKCE S256

The OAuth 2.0 flow is secured using **Proof Key for Code Exchange (PKCE)** with the `S256` code challenge method.

```mermaid
sequenceDiagram
    autonumber
    actor User as User Browser / Client App
    participant Core as CrowApi Backend - Port 8080
    participant Cache as Postgres cache_store
    participant Google as Google Identity Services
    participant DB as Postgres users & sessions

    Note over User,Core: Step 1: Authorization URL Request
    User->>Core: GET /api/v1/auth/google/url
    Core->>Core: Generate CSPRNG code_verifier (64 chars)<br/>Compute code_challenge = BASE64URL(SHA256(verifier))<br/>Generate random state token (22 chars)
    Core->>Cache: Set cache_key: "pkce:state:{state}", value: code_verifier (TTL: 600s)
    Core-->>User: 200 OK { authUrl, state, codeChallenge, codeChallengeMethod: "S256" }

    Note over User,Google: Step 2: User Authorization on Google
    User->>Google: Navigate to authUrl (with state, code_challenge, S256)
    User->>Google: Authenticate & Grant Consent
    Google-->>User: HTTP 302 Redirect to redirect_uri?code=4/0ATs...&state=...

    Note over User,Core: Step 3: Callback Processing & Single-Use Eviction
    User->>Core: GET /api/v1/auth/google/callback?code=...&state=...
    Core->>Cache: Query cache_key "pkce:state:{state}"
    alt State Not Found or Expired
        Cache-->>Core: 0 rows
        Core-->>User: 400 Bad Request { message: "Invalid or expired state" }
    else State Valid
        Cache-->>Core: code_verifier string
        Core->>Cache: DELETE FROM cache_store WHERE cache_key = "pkce:state:{state}"
        Note over Core,Cache: Replay Protection: code_verifier cannot be reused!
        
        Core->>Google: POST https://oauth2.googleapis.com/token<br/>(code, code_verifier, client_id, client_secret)
        Google-->>Core: 200 OK { id_token, access_token }
        
        Core->>Core: Verify Google ID Token (Signature & Claims via Google JWKS)<br/>Extract: google_id, email, avatar_url
        
        Core->>DB: Query user by email (jagasiva1999@gmail.com)
        alt Account Exists (Bidirectional Linking)
            DB-->>Core: Existing User Record
            Core->>DB: UPDATE users SET google_id = $1, avatar_url = $2, auth_provider = 'local+google'
        else New User Registration
            Core->>DB: INSERT INTO users (email, google_id, avatar_url, auth_provider) VALUES (...)
        end
        
        Core->>DB: INSERT INTO sessions (user_id, jti, refresh_token_hash, client_type, ...)
        Core->>Core: Issue RS256 JWT Access Token (expires in 15 mins)
        Core->>DB: INSERT INTO audit_logs ("AUTH_GOOGLE_SUCCESS")
        Core-->>User: 200 OK { accessToken, refreshToken, user: { email, role, avatarUrl } }
    end
```

---

### 1.4 Bidirectional Account Linking (`POST /api/v1/auth/set-password`)

When a user registers exclusively via Google OAuth, `password_hash` is initialized as `NULL`. The system allows users to set a local password to enable dual authentication:

```mermaid
sequenceDiagram
    autonumber
    actor Client
    participant AuthCtx as Middleware::AuthContext
    participant AuthCtrl as Presentation::AuthController
    participant AuthUC as Application::AuthUseCases
    participant Hasher as Infrastructure::OpenSslCrypto
    participant DB as Postgres users table

    Client->>AuthCtrl: POST /api/v1/auth/set-password { password }<br/>Header: Authorization: Bearer {accessToken}
    AuthCtrl->>AuthCtx: Validate JWT & Claims (sub: userId)
    AuthCtx-->>AuthCtrl: Authenticated Claims
    AuthCtrl->>AuthUC: setPassword(userId, newPassword)
    AuthUC->>Hasher: hashPassword(newPassword) [PBKDF2-HMAC-SHA256]
    Hasher-->>AuthUC: password_hash
    AuthUC->>DB: UPDATE users SET password_hash = $1, auth_provider = 'local+google' WHERE id = $2
    DB-->>AuthUC: OK
    AuthUC-->>AuthCtrl: Result::success()
    AuthCtrl-->>Client: 200 OK { message: "Password set successfully. You may now log in with either method." }
```

---

### 1.5 Refresh Token Rotation & Replay Attack Detection (`POST /api/v1/auth/refresh`)

To prevent stolen refresh tokens from remaining indefinitely valid, every refresh operation rotates the token and detects reuse:

```mermaid
sequenceDiagram
    autonumber
    actor Client
    participant AuthCtrl as Presentation::AuthController
    participant AuthUC as Application::AuthUseCases
    participant Hasher as Infrastructure::OpenSslCrypto
    participant SessionRepo as Infrastructure::PostgresSessionRepository
    participant Audit as Infrastructure::PostgresAuditLogRepository

    Client->>AuthCtrl: POST /api/v1/auth/refresh { refreshToken }
    AuthCtrl->>AuthUC: refreshToken(RefreshRequest)
    AuthUC->>Hasher: sha256(refreshToken)
    Hasher-->>AuthUC: submittedTokenHash

    AuthUC->>SessionRepo: findByPreviousRefreshHash(submittedTokenHash)
    alt Token Matches previous_refresh_token_hash (REPLAY ATTACK DETECTED!)
        SessionRepo-->>AuthUC: Session Entity Found
        Note over AuthUC,SessionRepo: CRITICAL SECURITY ALERT: An attacker or old client replayed a rotated token!
        AuthUC->>SessionRepo: revokeAllUserSessions(userId, "Token reuse detected - potential compromise")
        AuthUC->>Audit: recordEvent("SECURITY_ALERT_TOKEN_REPLAY", userId)
        AuthUC-->>AuthCtrl: Result::failure(TOKEN_COMPROMISED)
        AuthCtrl-->>Client: 401 Unauthorized { message: "Security violation: token reuse detected. All sessions revoked." }
    else No Replay Detected
        AuthUC->>SessionRepo: findByRefreshHash(submittedTokenHash)
        alt Token Not Found or Session Expired / Revoked
            SessionRepo-->>AuthUC: nullopt
            AuthUC-->>AuthCtrl: Result::failure(INVALID_TOKEN)
            AuthCtrl-->>Client: 401 Unauthorized
        else Valid Active Session
            AuthUC->>Hasher: generateSecureRandom(32 bytes) -> newRefreshToken
            AuthUC->>SessionRepo: rotateToken(sessionId, newHash, previousHash: submittedTokenHash, newJti)
            AuthUC-->>AuthCtrl: Result::success(AuthResponse)
            AuthCtrl-->>Client: 200 OK { accessToken: newJwt, refreshToken: newRefreshToken }
        end
    end
```

---

### 1.6 Real-Time Session Revocation via PostgreSQL `LISTEN / NOTIFY`

When an administrator revokes a session or a user clicks "Logout from All Devices", all connected API instances instantly blacklist the token in memory:

```mermaid
sequenceDiagram
    autonumber
    actor Admin
    participant CoreNodeA as CrowApi Node A
    participant DB as PostgreSQL 18.6
    participant Listener as Background jthread (Node B)
    participant CoreNodeB as CrowApi Node B

    Admin->>CoreNodeA: DELETE /api/v1/admin/sessions/{sessionId}
    CoreNodeA->>DB: UPDATE sessions SET revoked_at = now() WHERE id = $1 RETURNING jti;
    CoreNodeA->>DB: INSERT INTO token_revocations (jti, session_id, expires_at) VALUES (...);
    CoreNodeA->>DB: NOTIFY session_revoked, '{jti}';
    
    DB-->>Listener: Async Notification ('session_revoked', '{jti}')
    Listener->>CoreNodeB: Invalidate local in-memory JTI cache
    
    Note over CoreNodeB: Any future request bearing this JTI on Node B is instantly rejected with 401 Unauthorized!
```

---

## 2. Multi-Paradigm API Endpoints

### 2.1 Relational CRUD (`/api/todos`)
* `GET /api/todos`: Paginated task listing (`page`, `limit`).
* `POST /api/todos`: Create a new todo (`title`, `description`).
* `GET /api/todos/<id>`: Retrieve single todo.
* `PUT /api/todos/<id>`: Update title, description, or completed status.
* `DELETE /api/todos/<id>`: Delete todo.

### 2.2 Redis KV Cache Alternative (`/api/cache`)
* `GET /api/cache/<key>`: Fetch cached value (returns `404` if key does not exist or has expired).
* `POST /api/cache`: Set key-value pair with optional TTL (`key`, `value`, `ttlSeconds`).
* `DELETE /api/cache/<key>`: Explicitly delete a cache entry.
* `POST /api/cache/cleanup`: Trigger background cleanup of expired cache entries.

### 2.3 Kafka Queue Alternative (`/api/queue`)
* `POST /api/queue/publish`: Enqueue message (`topic`, `payload: { ... }`).
* `POST /api/queue/poll`: Drain pending messages with `FOR UPDATE SKIP LOCKED` (`topic`, `batchSize`).
* `POST /api/queue/ack/<id>`: Acknowledge successful processing (`status = 'PROCESSED'`).
* `POST /api/queue/fail/<id>`: Mark message as failed for retry handling (`status = 'FAILED'`).
* `GET /api/queue/metrics`: Queue depth and status distribution metrics.

### 2.4 MongoDB Document DB Alternative (`/api/documents`)
* `POST /api/documents/<collection>`: Store arbitrary schemaless JSON document.
* `GET /api/documents/<collection>/<id>`: Retrieve document by UUID.
* `POST /api/documents/<collection>/query`: Query collection using JSONB containment (`@>`) via GIN index.
* `PUT /api/documents/<collection>/<id>`: Replace or update document content.
* `DELETE /api/documents/<collection>/<id>`: Delete document.
