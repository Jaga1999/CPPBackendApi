# Complete Machine Setup, Build & Execution Guide

---

## 1. Operating System Prerequisites

### Windows (10 / 11)
1. **Visual Studio 2022 or 2026**:
   * Install the workload: **"Desktop development with C++"**.
   * Ensure components are checked: **MSVC v143/v144 toolset**, **C++20 standard library support**, **CMake tools for Windows**, and **C++ Clang Compiler for Windows** (optional).
2. **Git for Windows**: Download from [git-scm.com](https://git-scm.com/).
3. **Docker Desktop**: Download from [docker.com](https://www.docker.com/) (ensure the WSL2 backend is enabled).
4. **PowerShell 7+** or Windows PowerShell 5.1.
5. **vcpkg Package Manager**:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
   cd C:/vcpkg
   .\bootstrap-vcpkg.bat -disableMetrics
   ```

---

### Linux (Ubuntu 22.04 / 24.04, Debian 12)
Install the C++20 compiler toolchain, CMake, Ninja, OpenSSL, and PostgreSQL client libraries:
```bash
sudo apt update && sudo apt install -y \
    build-essential \
    g++-12 gcc-12 \
    cmake \
    ninja-build \
    git \
    curl \
    zip unzip tar \
    pkg-config \
    libssl-dev \
    libpq-dev \
    docker.io \
    docker-compose-v2

# Set GCC 12+ as default compiler if necessary
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
                        --slave /usr/bin/g++ g++ /usr/bin/g++-12

# Install standalone vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=~/vcpkg
```

---

### macOS (Apple Silicon M1/M2/M3/M4 & Intel)
1. **Xcode Command Line Tools**:
   ```bash
   xcode-select --install
   ```
2. **Homebrew**:
   ```bash
   brew install cmake ninja openssl@3 libpq git
   ```
3. **Docker Desktop for Mac**: Download from [docker.com](https://www.docker.com/).
4. **vcpkg**:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
   export VCPKG_ROOT=~/vcpkg
   ```

---

## 2. Clone the Repository

```bash
git clone https://github.com/Jaga1999/CPPBackendApi.git
cd CPPBackendApi
```
*(Or use the local workspace directory `e:\Projects\CPP\CrowApi`)*

---

## 3. Start PostgreSQL 18.6 Storage Engine

CrowApi uses **PostgreSQL 18.6** as a unified 4-in-1 multi-paradigm database. The repository provides a `docker-compose.yml` pre-configured to mount and run all migrations in `migrations/` automatically on startup:

```bash
# Start PostgreSQL container in background
docker-compose up -d postgres
```

### Verify Database Container Health:
```bash
docker ps --filter "name=crowapi-postgres"
```
You should see:
```text
CONTAINER ID   IMAGE             COMMAND                  STATUS                    PORTS
c99df60232fa   postgres:latest   "docker-entrypoint.s…"   Up 2 hours (healthy)      0.0.0.0:5432->5432/tcp
```

### Inspect Database Tables:
```bash
docker exec -it crowapi-postgres psql -U postgres -d crowapi_db -c "\dt"
```
Expected output showing the 4-in-1 multi-paradigm tables:
```text
              List of relations
 Schema |       Name        | Type  |  Owner   
--------+-------------------+-------+----------
 public | audit_logs        | table | postgres
 public | cache_store       | table | postgres
 public | documents         | table | postgres
 public | message_queue     | table | postgres
 public | sessions          | table | postgres
 public | todos             | table | postgres
 public | token_revocations | table | postgres
 public | users             | table | postgres
(8 rows)
```

---

## 4. Environment Configuration

### Step 4.1: Server Runtime Configuration (`.env`)
Copy the provided `.env.example` to `.env`:
```bash
cp .env.example .env
```
Edit `.env` to configure your settings:
```ini
# Server Settings
PORT=8080
LOG_LEVEL=debug
THREADS=16

# PostgreSQL Connection Pool
DB_HOST=localhost
DB_PORT=5432
DB_NAME=crowapi_db
DB_USER=postgres
DB_PASSWORD=postgres
DB_POOL_SIZE=20

# JWT & RS256 Cryptography
JWT_KID=key-2026-prod-01
JWT_EXPIRATION_SECONDS=900
REFRESH_TOKEN_EXPIRATION_DAYS=30

# Google OAuth 2.0 / OIDC (Optional for Google Sign-In)
GOOGLE_CLIENT_ID=your-google-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=GOCSPX-your-google-client-secret
GOOGLE_REDIRECT_URI=http://localhost:8080/api/v1/auth/google/callback
```

### Step 4.2: Automated Test Configuration (`.env.test`)
The test runner automatically loads `.env.test`:
```ini
PORT=8081
LOG_LEVEL=warning
DB_HOST=localhost
DB_PORT=5432
DB_NAME=crowapi_db
DB_USER=postgres
DB_PASSWORD=postgres
DB_POOL_SIZE=10
JWT_KID=test-key-2026
JWT_EXPIRATION_SECONDS=900
REFRESH_TOKEN_EXPIRATION_DAYS=30
GOOGLE_CLIENT_ID=mock-google-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=mock-google-client-secret
GOOGLE_REDIRECT_URI=http://localhost:8080/api/v1/auth/google/callback
```

---

## 5. Building the Solution

### Option A: Using CMake Presets (Windows / Visual Studio)
The repository includes a standardized `CMakePresets.json`:
```powershell
# 1. Configure the project with x64 Debug preset
cmake --preset x64-debug

# 2. Compile both the Core server and Tests executable
cmake --build --preset x64-debug
```

For optimized Release builds:
```powershell
cmake --preset x64-release
cmake --build --preset x64-release
```

---

### Option B: Command-Line CMake with Ninja (Linux & macOS)
```bash
# 1. Configure build directory with C++20 and vcpkg toolchain
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# 2. Build the Core executable and Test Suite
cmake --build build --target Core Tests
```

---

### Option C: Complete Dockerized Production Build (Zero Local Toolchain)
If you do not want to install C++ compilers, CMake, or vcpkg locally, build and run the entire stack via Docker:
```bash
docker-compose up -d --build
```
This multi-stage Docker build will:
1. Compile the C++20 native binary inside an Ubuntu builder stage using Ninja.
2. Produce a minimal, non-root runtime container (~45 MB).
3. Connect the application container to PostgreSQL on the internal bridge network.

---

## 6. Running the Automated Test Suite

Before running the server, execute the comprehensive test suite to verify cryptographic compliance, database connections, and security defenses:

### Windows:
```powershell
.\out\build\x64-debug\Tests\Tests.exe
```

### Linux / macOS:
```bash
./build/Tests/Tests
```

### Running Specific Test Categories:
The custom test runner supports filtering via `--filter=` or `-f=`:
```powershell
# Run only Cyber-Attack penetration tests (SQLi, XSS, JWT tampering, Replay)
.\out\build\x64-debug\Tests\Tests.exe --filter=CyberAttacks

# Run only PKCE and RFC 7636 Appendix B test vectors
.\out\build\x64-debug\Tests\Tests.exe --filter=PKCE

# Run only Concurrency & Performance benchmarks
.\out\build\x64-debug\Tests\Tests.exe --filter=Performance

# Run only PostgreSQL multi-paradigm repository tests
.\out\build\x64-debug\Tests\Tests.exe --filter=Postgres
```

### Expected Output:
```text
=================================================================
  CrowApi Comprehensive Multi-Layer Test Suite                    
  Environment  : .env.test (PostgreSQL 18.6 + OpenSSL 3.x)         
  Total Tests  : 68
=================================================================
--- [Domain::Result] ---
  [PASS] SuccessResultHoldsValue (4 us)
  [PASS] FailureResultHoldsError (5 us)
...
=================================================================
  TEST EXECUTION SUMMARY
  Total Executed : 68 | Passed : 68 | Failed : 0 | Assertions : 435
  Total Time     : ~3000 ms
=================================================================
>>> OVERALL RESULT: ALL TESTS PASSED <<<
```

---

## 7. Starting the API Server

### Running Native Binary (Windows):
```powershell
.\out\build\x64-debug\Core\Core.exe
```
Or specify a custom port and log level via CLI arguments:
```powershell
.\out\build\x64-debug\Core\Core.exe 8080 debug
```

### Running Native Binary (Linux / macOS):
```bash
./build/Core/Core 8080 info
```

### Server Startup Banner:
```text
[EnvLoader] Loaded 19 variables from: .env
=================================================================
  Crow Modern C++20 REST API Service                             
  Architecture : Clean Architecture (Multi-Module CMake)         
  Database     : PostgreSQL 18.6 (4-in-1 Multi-Paradigm Engine)   
  Port         : 8080
  Log Level    : debug
  Threads      : 16
  Key ID       : key-2026-prod-01
=================================================================
[PostgresDb] Connecting to: host=localhost port=5432 dbname=crowapi_db (pool size: 20)...
[RsaKeyManager] Generating initial RSA 2048-bit keypair for kid: key-2026-prod-01...
(2026-09-05 11:42:11) [INFO    ] Crow/master server is running at http://0.0.0.0:8080 using 16 threads
[PostgresSessionRevocationListener] Listening on channel 'session_revoked'...
```

---

## 8. Verifying & Interacting with the Live API

### 8.1 Health Check & Documentation
* **Health Check**:
  ```bash
  curl http://localhost:8080/health
  ```
* **Interactive Swagger UI**:
  Open in your web browser: **`http://localhost:8080/docs`**
* **OpenAPI 3.1 Spec**:
  ```bash
  curl http://localhost:8080/api/openapi.json
  ```
* **Public JWKS Discovery**:
  ```bash
  curl http://localhost:8080/.well-known/jwks.json
  ```

---

### 8.2 Testing Local Authentication Lifecycle

#### 1. Register a New Account:

**Bash (cURL):**
```bash
curl -s -X POST http://localhost:8080/api/v1/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email": "developer@example.com",
    "password": "StrongPassword123!"
  }'
```

**PowerShell:**
```powershell
$registerBody = @{
    email = "developer@example.com"
    password = "StrongPassword123!"
} | ConvertTo-Json

Invoke-RestMethod -Uri "http://localhost:8080/api/v1/auth/register" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body $registerBody
```

---

#### 2. Log in and Receive RS256 JWT & Refresh Token:

**Bash (cURL):**
```bash
# Login and capture access token and refresh token
RESPONSE=$(curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "developer@example.com",
    "password": "StrongPassword123!",
    "deviceName": "Bash Terminal"
  }')

echo "$RESPONSE"
TOKEN=$(echo "$RESPONSE" | grep -o '"accessToken":"[^"]*' | cut -d'"' -f4)
REFRESH_TOKEN=$(echo "$RESPONSE" | grep -o '"refreshToken":"[^"]*' | cut -d'"' -f4)
echo "JWT Access Token: $TOKEN"
```

**PowerShell:**
```powershell
$loginBody = @{
    email = "developer@example.com"
    password = "StrongPassword123!"
    deviceName = "PowerShell Client"
} | ConvertTo-Json

$authResponse = Invoke-RestMethod -Uri "http://localhost:8080/api/v1/auth/login" `
                                  -Method Post `
                                  -ContentType "application/json" `
                                  -Body $loginBody

$token = $authResponse.data.accessToken
$refreshToken = $authResponse.data.refreshToken
Write-Host "Received JWT Access Token: $token"
```

---

#### 3. Call Authenticated Endpoint with Bearer Token:

**Bash (cURL):**
```bash
curl -s -X GET http://localhost:8080/api/v1/sessions \
  -H "Authorization: Bearer $TOKEN"
```

**PowerShell:**
```powershell
Invoke-RestMethod -Uri "http://localhost:8080/api/v1/sessions" `
                  -Headers @{ Authorization = "Bearer $token" }
```

---

#### 4. Refresh Token (Rotates both tokens):

**Bash (cURL):**
```bash
curl -s -X POST http://localhost:8080/api/v1/auth/refresh \
  -H "Content-Type: application/json" \
  -d "{\"refreshToken\": \"$REFRESH_TOKEN\"}"
```

**PowerShell:**
```powershell
$refreshBody = @{ refreshToken = $refreshToken } | ConvertTo-Json
$refreshed = Invoke-RestMethod -Uri "http://localhost:8080/api/v1/auth/refresh" `
                               -Method Post `
                               -ContentType "application/json" `
                               -Body $refreshBody
```

---

#### 5. Logout & Revoke Session:

**Bash (cURL):**
```bash
curl -s -X POST http://localhost:8080/api/v1/auth/logout \
  -H "Authorization: Bearer $TOKEN"
```

**PowerShell:**
```powershell
Invoke-RestMethod -Uri "http://localhost:8080/api/v1/auth/logout" `
                  -Method Post `
                  -Headers @{ Authorization = "Bearer $token" }
```

---

### 8.3 Testing Google Sign-In Flow (RFC 7636 PKCE S256)

1. Request an authorization URL with generated PKCE challenge:

**Bash (cURL):**
```bash
curl -s http://localhost:8080/api/v1/auth/google/url
```

**PowerShell:**
```powershell
$urlRes = Invoke-RestMethod -Uri "http://localhost:8080/api/v1/auth/google/url"
Write-Host "Open this URL in your browser:"
Write-Host $urlRes.data.authUrl
```

2. Open the returned `authUrl` in your web browser.
3. Authenticate with your Google account and grant consent.
4. Google redirects to:
   ```text
   http://localhost:8080/api/v1/auth/google/callback?code=4/0ATs...&state=...
   ```
5. The backend validates the PKCE code challenge against the cached verifier, creates your session, and returns your JWT access token and refresh token.

---

### 8.4 Testing Multi-Paradigm PostgreSQL Endpoints

#### 1. Relational SQL Database (`todos`)

**Bash (cURL):**
```bash
# Create a todo
curl -s -X POST http://localhost:8080/api/todos \
  -H "Content-Type: application/json" \
  -d '{"title": "Complete C++20 Project", "description": "Native Clean Architecture API"}'

# List todos
curl -s http://localhost:8080/api/todos
```

**PowerShell:**
```powershell
# Create a todo
Invoke-RestMethod -Uri "http://localhost:8080/api/todos" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body '{"title": "Complete C++20 Project", "description": "Native Clean Architecture API"}'

# List todos
Invoke-RestMethod -Uri "http://localhost:8080/api/todos"
```

---

#### 2. Redis KV Cache Alternative (`cache_store`)

**Bash (cURL):**
```bash
# Store key with 60-second TTL
curl -s -X POST http://localhost:8080/api/cache \
  -H "Content-Type: application/json" \
  -d '{"key": "demo_key", "value": "demo_value", "ttlSeconds": 60}'

# Read key
curl -s http://localhost:8080/api/cache/demo_key
```

**PowerShell:**
```powershell
# Store key with 60-second TTL
Invoke-RestMethod -Uri "http://localhost:8080/api/cache" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body '{"key": "demo_key", "value": "demo_value", "ttlSeconds": 60}'

# Read key
Invoke-RestMethod -Uri "http://localhost:8080/api/cache/demo_key"
```

---

#### 3. Kafka Message Queue Alternative (`message_queue`)

**Bash (cURL):**
```bash
# Publish message
curl -s -X POST http://localhost:8080/api/queue/publish \
  -H "Content-Type: application/json" \
  -d '{"topic": "order_events", "payload": {"orderId": "1001", "amount": 49.99}}'

# Drain pending messages via FOR UPDATE SKIP LOCKED
curl -s -X POST http://localhost:8080/api/queue/poll \
  -H "Content-Type: application/json" \
  -d '{"topic": "order_events", "batchSize": 5}'
```

**PowerShell:**
```powershell
# Publish message
Invoke-RestMethod -Uri "http://localhost:8080/api/queue/publish" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body '{"topic": "order_events", "payload": {"orderId": "1001", "amount": 49.99}}'

# Drain pending messages via FOR UPDATE SKIP LOCKED
Invoke-RestMethod -Uri "http://localhost:8080/api/queue/poll" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body '{"topic": "order_events", "batchSize": 5}'
```

---

#### 4. MongoDB Document DB Alternative (`documents`)

**Bash (cURL):**
```bash
# Store schemaless JSON document
curl -s -X POST http://localhost:8080/api/documents/users \
  -H "Content-Type: application/json" \
  -d '{"name": "Alice", "preferences": {"theme": "dark", "notifications": true}}'

# Query using JSONB @> containment via GIN index
curl -s -X POST http://localhost:8080/api/documents/users/query \
  -H "Content-Type: application/json" \
  -d '{"preferences": {"theme": "dark"}}'
```

**PowerShell:**
```powershell
# Store schemaless JSON document
Invoke-RestMethod -Uri "http://localhost:8080/api/documents/users" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body '{"name": "Alice", "preferences": {"theme": "dark", "notifications": true}}'

# Query using JSONB @> containment via GIN index
Invoke-RestMethod -Uri "http://localhost:8080/api/documents/users/query" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body '{"preferences": {"theme": "dark"}}'
```

---

## 9. Troubleshooting & Common Questions

| Problem | Cause | Solution |
| :--- | :--- | :--- |
| **Port 8080 is already in use** | Another instance or process is occupying port 8080. | Windows: `Get-Process -Id (Get-NetTCPConnection -LocalPort 8080).OwningProcess \| Stop-Process -Force`. Linux: `fuser -k 8080/tcp`. |
| **Database Connection Refused** | PostgreSQL container is not running or still initializing. | Run `docker-compose up -d postgres` and wait 5 seconds for the health check to turn healthy (`docker ps`). |
| **`vcpkg.cmake` not found during CMake configure** | `CMAKE_TOOLCHAIN_FILE` path does not match your vcpkg installation path. | Pass explicit toolchain path: `cmake -B build -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake`. |
| **401 Unauthorized on Google Callback** | Google OAuth Client ID/Secret missing or state expired. | Check `.env` contains valid `GOOGLE_CLIENT_ID` and `GOOGLE_CLIENT_SECRET`. Complete callback within 10 minutes (TTL). |
