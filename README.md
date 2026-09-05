# CrowApi - Modern C++20 Clean Architecture REST API

An enterprise-grade, modern C++20 REST API built with the **Crow framework** and powered by **PostgreSQL 18.6** as a unified 4-in-1 multi-paradigm backend (Relational SQL, Redis-like KV cache, Kafka-like transactional message queue, and MongoDB-like JSONB document store), alongside a production-ready **Pure JWT / JWK Authentication & Multi-Session Subsystem**.

---

## Key Features

1. **Pure Stateless JWT Authentication (No HTTP Sessions)**:
   - Zero stateful cookies or session cookies. Every protected request uses `Authorization: Bearer <token>`.
   - Asymmetric RS256 signing using RSA 2048-bit keys with public keys discoverable via RFC 7517 JWKS (`GET /.well-known/jwks.json`).
   - Refresh token rotation with automatic replay/reuse detection that revokes all sessions in the family upon detected reuse.
   - Brute-force protection: accounts automatically lock for 15 minutes after 5 failed consecutive attempts.

2. **Multi-Session Device Registry & JTI-Based Revocation**:
   - "Sessions" are stored device logins recording client IP, User-Agent, device name, client type, and unique `jti`.
   - Users can list and inspect their active devices (`GET /api/v1/sessions`).
   - Revoking a session bans the specific JWT by writing its `jti` to `token_revocations`. Any banned token is instantly rejected with `401 Unauthorized`.
   - Real-time cross-node notification via PostgreSQL `LISTEN`/`NOTIFY` on channel `session_revoked`.

3. **Unified PostgreSQL 18.6 (4-in-1 Multi-Paradigm Engine)**:
   - **Relational SQL**: ACID CRUD operations on `todos`.
   - **Redis Alternative**: In-database KV cache with TTL eviction (`cache_store`).
   - **Kafka Alternative**: Transactional queue with atomic `FOR UPDATE SKIP LOCKED` concurrency (`message_queue`).
   - **MongoDB Alternative**: Schemaless `JSONB` documents with GIN indexing (`documents`).

4. **Performance Optimizations**:
   - HOT (Heap-Only Tuples) update optimization with `fillfactor = 85` on `sessions` and `cache_store`, `fillfactor = 80` on `message_queue`.
   - Partial indexes (`WHERE revoked_at IS NULL`, `WHERE status = 'PENDING'`).
   - Fast JSON containment queries with `jsonb_path_ops` GIN indexing.
   - Thread-safe connection pool with RAII leasing and backpressure.

5. **Clean Architecture Multi-Module Subprojects**:
   - `Domain`: Enterprise business entities, domain errors, and repository interfaces.
   - `Application`: Use cases, input validation, DTOs, and OpenAPI specification.
   - `Infrastructure`: PostgreSQL 18.6 persistence, OpenSSL crypto, RSA key management, JWT services.
   - `Presentation`: Crow web controllers, HTTP serialization, and logging middleware.
   - `Core`: Composition root, dependency injection, and CLI runner.

6. **Compiler Cleanliness & Tooling**:
   - Strictly ISO C++20 (`/std:c++20 /permissive-`).
   - **0 Warnings, 0 Errors** on CMake & MSVC.
   - Automated runtime DLL deployment on Windows.
   - OpenAPI 3.1 & interactive Swagger UI at `/docs`.

---

## API Endpoints Overview

### Authentication & Sessions
- `POST /api/v1/auth/register` - User registration
- `POST /api/v1/auth/login` - User login (issues access JWT + refresh token)
- `POST /api/v1/auth/refresh` - Refresh token rotation
- `POST /api/v1/auth/logout` - Logout (revokes current session & bans JWT via `jti`)
- `GET  /.well-known/jwks.json` - RFC 7517 JWKS discovery
- `GET  /api/v1/sessions` - List user's active device sessions
- `DELETE /api/v1/sessions/<id>` - Revoke specific device session (bans its `jti`)
- `DELETE /api/v1/sessions` - Revoke all sessions (except current)
- `GET  /api/v1/admin/sessions` - Admin session inspection (with filtering)
- `DELETE /api/v1/admin/sessions/<id>` - Admin session revocation
- `DELETE /api/v1/admin/users/<userId>/sessions` - Admin revoke all user sessions

### 4-in-1 PostgreSQL Engine
- **Relational SQL**: `GET/POST /api/todos`, `GET/PUT/DELETE /api/todos/<id>`
- **KV Cache**: `GET/POST/DELETE /api/cache/<key>`, `POST /api/cache/cleanup`
- **Message Queue**: `POST /api/queue/publish`, `POST /api/queue/poll`, `POST /api/queue/ack/<id>`, `GET /api/queue/metrics`
- **JSONB Documents**: `POST /api/documents/<col>`, `GET/PUT/DELETE /api/documents/<col>/<id>`, `POST /api/documents/<col>/query`

### Documentation & Health
- `GET /health` - Health check & uptime
- `GET /docs` - Interactive Swagger UI
- `GET /api/openapi.json` - OpenAPI 3.1 specification

---

## Build & Run

### Prerequisites
- Visual Studio 2022/2026 with C++20 toolchain
- CMake 3.20+
- PostgreSQL 18.6 (Docker or local instance)

### Build with CMake
```powershell
cmake -B out/build/x64-debug -S .
cmake --build out/build/x64-debug
```

### Run Server
```powershell
# Default port 8080, info logging
.\out\build\x64-debug\Core\Core.exe 8080 debug
```
