# Security Policy & Architecture

For full technical specifications, architecture diagrams, sequence workflows, and threat matrices, please see the complete **[CrowApi Security Architecture Documentation](docs/security.md)**.

---

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |

---

## Reporting a Vulnerability

If you discover a security vulnerability within CrowApi, please follow these responsible disclosure guidelines:

1. **Do not open a public issue.**
2. Send an email to the security team or repository owner with details of the vulnerability.
3. Include the following information:
   - Type of vulnerability (e.g. CSRF, SQL Injection, Buffer Overrun, JWT bypass)
   - Step-by-step reproduction instructions or a proof of concept (PoC)
   - Affected endpoints or components
   - Impact assessment

We will acknowledge receipt within 48 hours and provide updates on resolution and patches.

---

## Security Architectural Highlights

* **Pure Stateless Asymmetric RS256 JWTs**: Cryptographically signed using RSA 2048-bit keys via OpenSSL 3.x; public keys discoverable via RFC 7517 JWKS at `/.well-known/jwks.json`.
* **RFC 7636 PKCE S256**: All Google OAuth flows enforce Proof Key for Code Exchange using cryptographically random 64-character verifiers and SHA-256 base64url challenges with single-use replay eviction.
* **Refresh Token Family Revocation**: Replay attacks on rotated refresh tokens automatically revoke the entire family of active user sessions.
* **Real-Time Revocation**: Instantaneous cluster-wide token blacklisting using PostgreSQL `LISTEN / NOTIFY` on channel `session_revoked`.
* **100% Parameterized SQL**: Zero string concatenation in database queries via `pqxx::params`.
* **Brute-Force Lockout Defense**: 5 consecutive failed login attempts trigger an automatic 15-minute account lockout.
