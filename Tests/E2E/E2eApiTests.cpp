#include "TestHarness.h"
#include "Presentation/Routes/Router.h"
#include "Infrastructure/Config/EnvLoader.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include "Infrastructure/Persistence/PostgresTodoRepository.h"
#include "Infrastructure/Persistence/PostgresCacheRepository.h"
#include "Infrastructure/Persistence/PostgresMessageQueueRepository.h"
#include "Infrastructure/Persistence/PostgresDocumentRepository.h"
#include "Infrastructure/Persistence/PostgresUserRepository.h"
#include "Infrastructure/Persistence/PostgresSessionRepository.h"
#include "Infrastructure/Persistence/PostgresAuditLogRepository.h"
#include "Infrastructure/Security/OpenSslCryptoAdapters.h"
#include "Infrastructure/Security/RsaKeyManager.h"
#include "Infrastructure/Security/JwtService.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include "Infrastructure/Security/GoogleAuthService.h"
#include "Application/Security/AuthorizationService.h"
#include "Application/UseCases/TodoUseCases.h"
#include "Application/UseCases/CacheUseCases.h"
#include "Application/UseCases/QueueUseCases.h"
#include "Application/UseCases/DocumentUseCases.h"
#include "Application/UseCases/AuthUseCases.h"
#include "Application/UseCases/SessionUseCases.h"

#include <crow/json.h>

struct TestServerContext {
    Presentation::Routes::AppType app;
    std::shared_ptr<Infrastructure::Persistence::PostgresDb> db;
    std::shared_ptr<Infrastructure::Security::RsaKeyManager> keyMgr;
    std::shared_ptr<Infrastructure::Security::JwtService> jwtService;
    std::shared_ptr<Infrastructure::Persistence::PostgresUserRepository> userRepo;
    std::shared_ptr<Infrastructure::Persistence::PostgresSessionRepository> sessionRepo;
    std::shared_ptr<Infrastructure::Persistence::PostgresTodoRepository> todoRepo;
    std::shared_ptr<Infrastructure::Persistence::PostgresCacheRepository> cacheRepo;
    std::shared_ptr<Infrastructure::Persistence::PostgresMessageQueueRepository> queueRepo;
    std::shared_ptr<Infrastructure::Persistence::PostgresDocumentRepository> docRepo;
    std::shared_ptr<Infrastructure::Persistence::PostgresAuditLogRepository> auditLogRepo;

    std::shared_ptr<Infrastructure::Security::OpenSslPasswordHasher> passwordHasher;
    std::shared_ptr<Infrastructure::Security::OpenSslTokenGenerator> tokenGenerator;
    std::shared_ptr<Infrastructure::Security::GoogleAuthService> googleAuthService;
    std::shared_ptr<Application::Security::AuthorizationService> authService;

    std::shared_ptr<Application::UseCases::CreateTodoUseCase> createTodoUseCase;
    std::shared_ptr<Application::UseCases::GetTodoByIdUseCase> getTodoByIdUseCase;
    std::shared_ptr<Application::UseCases::ListTodosUseCase> listTodosUseCase;
    std::shared_ptr<Application::UseCases::UpdateTodoUseCase> updateTodoUseCase;
    std::shared_ptr<Application::UseCases::DeleteTodoUseCase> deleteTodoUseCase;

    std::shared_ptr<Application::UseCases::CacheUseCases> cacheUseCases;
    std::shared_ptr<Application::UseCases::QueueUseCases> queueUseCases;
    std::shared_ptr<Application::UseCases::DocumentUseCases> docUseCases;

    std::shared_ptr<Application::UseCases::AuthUseCases> authUseCases;
    std::shared_ptr<Application::UseCases::SessionUseCases> sessionUseCases;

    std::unique_ptr<Presentation::Controllers::HealthController> healthController;
    std::unique_ptr<Presentation::Controllers::TodoController> todoController;
    std::unique_ptr<Presentation::Controllers::CacheController> cacheController;
    std::unique_ptr<Presentation::Controllers::QueueController> queueController;
    std::unique_ptr<Presentation::Controllers::DocumentController> documentController;
    std::unique_ptr<Presentation::Controllers::DocsController> docsController;
    std::unique_ptr<Presentation::Controllers::AuthController> authController;
    std::unique_ptr<Presentation::Controllers::SessionController> sessionController;
    std::unique_ptr<Presentation::Controllers::AdminSessionController> adminSessionController;
};

static TestServerContext& getTestServer() {
    static bool initialized = false;
    static TestServerContext ctx;

    if (!initialized) {
        ctx.db = std::make_shared<Infrastructure::Persistence::PostgresDb>();
        ctx.todoRepo = std::make_shared<Infrastructure::Persistence::PostgresTodoRepository>(ctx.db);
        ctx.cacheRepo = std::make_shared<Infrastructure::Persistence::PostgresCacheRepository>(ctx.db);
        ctx.queueRepo = std::make_shared<Infrastructure::Persistence::PostgresMessageQueueRepository>(ctx.db);
        ctx.docRepo = std::make_shared<Infrastructure::Persistence::PostgresDocumentRepository>(ctx.db);
        ctx.userRepo = std::make_shared<Infrastructure::Persistence::PostgresUserRepository>(ctx.db);
        ctx.sessionRepo = std::make_shared<Infrastructure::Persistence::PostgresSessionRepository>(ctx.db);
        ctx.auditLogRepo = std::make_shared<Infrastructure::Persistence::PostgresAuditLogRepository>(ctx.db);

        ctx.passwordHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
        ctx.tokenGenerator = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
        ctx.keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("e2e-key-2026");
        ctx.jwtService = std::make_shared<Infrastructure::Security::JwtService>(ctx.keyMgr, "crow-api-auth", "crow-api-clients");
        ctx.authService = std::make_shared<Application::Security::AuthorizationService>();

        ctx.createTodoUseCase = std::make_shared<Application::UseCases::CreateTodoUseCase>(ctx.todoRepo);
        ctx.getTodoByIdUseCase = std::make_shared<Application::UseCases::GetTodoByIdUseCase>(ctx.todoRepo);
        ctx.listTodosUseCase = std::make_shared<Application::UseCases::ListTodosUseCase>(ctx.todoRepo);
        ctx.updateTodoUseCase = std::make_shared<Application::UseCases::UpdateTodoUseCase>(ctx.todoRepo);
        ctx.deleteTodoUseCase = std::make_shared<Application::UseCases::DeleteTodoUseCase>(ctx.todoRepo);

        ctx.cacheUseCases = std::make_shared<Application::UseCases::CacheUseCases>(ctx.cacheRepo);
        ctx.queueUseCases = std::make_shared<Application::UseCases::QueueUseCases>(ctx.queueRepo);
        ctx.docUseCases = std::make_shared<Application::UseCases::DocumentUseCases>(ctx.docRepo);

        ctx.googleAuthService = std::make_shared<Infrastructure::Security::GoogleAuthService>(
            "842447367765-frrh5uml3a0hh8cciidl4fk4u23bltql.apps.googleusercontent.com",
            "GOCSPX-mock-secret",
            "http://localhost:8080/api/v1/auth/google/callback"
        );

        ctx.authUseCases = std::make_shared<Application::UseCases::AuthUseCases>(
            ctx.userRepo, ctx.sessionRepo, ctx.auditLogRepo, ctx.jwtService, ctx.passwordHasher, ctx.tokenGenerator,
            ctx.googleAuthService, ctx.cacheRepo);
        ctx.sessionUseCases = std::make_shared<Application::UseCases::SessionUseCases>(
            ctx.sessionRepo, ctx.userRepo, ctx.auditLogRepo, ctx.authService);

        ctx.healthController = std::make_unique<Presentation::Controllers::HealthController>();
        ctx.todoController = std::make_unique<Presentation::Controllers::TodoController>(
            ctx.createTodoUseCase, ctx.getTodoByIdUseCase, ctx.listTodosUseCase, ctx.updateTodoUseCase, ctx.deleteTodoUseCase);
        ctx.cacheController = std::make_unique<Presentation::Controllers::CacheController>(ctx.cacheUseCases);
        ctx.queueController = std::make_unique<Presentation::Controllers::QueueController>(ctx.queueUseCases);
        ctx.documentController = std::make_unique<Presentation::Controllers::DocumentController>(ctx.docUseCases);
        ctx.docsController = std::make_unique<Presentation::Controllers::DocsController>();
        ctx.authController = std::make_unique<Presentation::Controllers::AuthController>(
            ctx.authUseCases, ctx.keyMgr, ctx.jwtService, ctx.sessionRepo, ctx.userRepo);
        ctx.sessionController = std::make_unique<Presentation::Controllers::SessionController>(
            ctx.sessionUseCases, ctx.jwtService, ctx.sessionRepo, ctx.userRepo);
        ctx.adminSessionController = std::make_unique<Presentation::Controllers::AdminSessionController>(
            ctx.sessionUseCases, ctx.jwtService, ctx.sessionRepo, ctx.userRepo);

        Presentation::Routes::Router::registerRoutes(
            ctx.app,
            *ctx.healthController,
            *ctx.todoController,
            *ctx.cacheController,
            *ctx.queueController,
            *ctx.documentController,
            *ctx.docsController,
            *ctx.authController,
            *ctx.sessionController,
            *ctx.adminSessionController
        );

        ctx.app.validate();
        initialized = true;
    }
    return ctx;
}

// 1. Health & OpenAPI Endpoints
TEST_CASE("E2E::Endpoints", "HealthAndDocsEndpoint") {
    auto& server = getTestServer();

    crow::request req;
    req.url = "/health";
    req.method = crow::HTTPMethod::Get;

    crow::response res;
    server.app.handle_full(req, res);

    EXPECT_EQ(res.code, 200);
    auto json = crow::json::load(res.body);
    EXPECT_TRUE(json);
    EXPECT_TRUE(json["success"].b());
}

// 2. JWKS RFC 7517 Discovery
TEST_CASE("E2E::Endpoints", "JwksDiscoveryEndpoint") {
    auto& server = getTestServer();

    crow::request req;
    req.url = "/.well-known/jwks.json";
    req.method = crow::HTTPMethod::Get;

    crow::response res;
    server.app.handle_full(req, res);

    EXPECT_EQ(res.code, 200);
    auto json = crow::json::load(res.body);
    EXPECT_TRUE(json);
    EXPECT_TRUE(json.has("keys"));
    EXPECT_TRUE(json["keys"].size() >= 1);
}

// 3. User Registration, Login, Session Inspection, and Logout Flow
TEST_CASE("E2E::AuthFlow", "FullAuthenticationAndSessionLifecycle") {
    auto& server = getTestServer();

    std::string uniqueId = Infrastructure::Security::OpenSslCrypto::generateSecureToken(6);
    std::string email = "e2e_alice_" + uniqueId + "@example.com";
    std::string password = "StrongPassword2026!";

    // Step 1: Register
    {
        crow::request req;
        req.url = "/api/v1/auth/register";
        req.method = crow::HTTPMethod::Post;
        req.body = "{\"email\":\"" + email + "\",\"password\":\"" + password + "\"}";
        req.add_header("Content-Type", "application/json");

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 201);
    }

    // Step 2: Login Device 1 (iPhone)
    std::string accessToken;
    std::string refreshToken;
    std::string jti;
    {
        crow::request req;
        req.url = "/api/v1/auth/login";
        req.method = crow::HTTPMethod::Post;
        req.body = "{\"email\":\"" + email + "\",\"password\":\"" + password + "\",\"device_name\":\"Alice iPhone\",\"client_type\":\"mobile\"}";
        req.add_header("Content-Type", "application/json");
        req.add_header("User-Agent", "iPhone iOS 18");

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 200);

        auto json = crow::json::load(res.body);
        EXPECT_TRUE(json);
        accessToken = json["data"]["accessToken"].s();
        refreshToken = json["data"]["refreshToken"].s();
        EXPECT_FALSE(accessToken.empty());
        EXPECT_FALSE(refreshToken.empty());

        auto claimsRes = server.jwtService->validateAccessToken(accessToken);
        EXPECT_TRUE(claimsRes.isSuccess());
        jti = claimsRes.value().jti;
    }

    // Step 3: Inspect active sessions (GET /api/v1/sessions)
    {
        crow::request req;
        req.url = "/api/v1/sessions";
        req.method = crow::HTTPMethod::Get;
        req.add_header("Authorization", "Bearer " + accessToken);

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 200);

        auto json = crow::json::load(res.body);
        EXPECT_TRUE(json);
        EXPECT_TRUE(json["data"].size() >= 1);
        EXPECT_TRUE(json["data"][0]["current"].b());
    }

    // Step 4: Refresh Token Rotation (POST /api/v1/auth/refresh)
    std::string rotatedAccessToken;
    {
        crow::request req;
        req.url = "/api/v1/auth/refresh";
        req.method = crow::HTTPMethod::Post;
        req.body = "{\"refreshToken\":\"" + refreshToken + "\"}";
        req.add_header("Content-Type", "application/json");

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 200);

        auto json = crow::json::load(res.body);
        EXPECT_TRUE(json);
        rotatedAccessToken = json["data"]["accessToken"].s();
        EXPECT_FALSE(rotatedAccessToken.empty());
    }

    // Step 5: Logout (POST /api/v1/auth/logout)
    {
        crow::request req;
        req.url = "/api/v1/auth/logout";
        req.method = crow::HTTPMethod::Post;
        req.add_header("Authorization", "Bearer " + rotatedAccessToken);

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 200);
    }

    // Step 6: Verify token is now banned & rejected with 401
    {
        crow::request req;
        req.url = "/api/v1/sessions";
        req.method = crow::HTTPMethod::Get;
        req.add_header("Authorization", "Bearer " + rotatedAccessToken);

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 401);
    }
}

// 4. Multi-Paradigm Engine End-to-End Test
TEST_CASE("E2E::MultiParadigm", "RelationalSqlAndKvCacheAndQueueAndDocuments") {
    auto& server = getTestServer();

    // 1. Relational SQL Todo Create & Fetch
    uint64_t todoId = 0;
    {
        crow::request req;
        req.url = "/api/todos";
        req.method = crow::HTTPMethod::Post;
        req.body = "{\"title\":\"E2E Workflow Todo\",\"description\":\"Verify clean arch\"}";
        req.add_header("Content-Type", "application/json");

        crow::response res;
        server.app.handle_full(req, res);
        EXPECT_EQ(res.code, 201);

        auto json = crow::json::load(res.body);
        EXPECT_TRUE(json);
        todoId = json["data"]["id"].i();
        EXPECT_TRUE(todoId > 0);
    }

    // 2. Redis KV Cache Set & Get
    std::string cacheKey = "e2e:cache:" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(6);
    {
        crow::request setReq;
        setReq.url = "/api/cache";
        setReq.method = crow::HTTPMethod::Post;
        setReq.body = "{\"key\":\"" + cacheKey + "\",\"value\":\"e2e_cached_data\",\"ttl\":300}";
        setReq.add_header("Content-Type", "application/json");

        crow::response setRes;
        server.app.handle_full(setReq, setRes);
        EXPECT_EQ(setRes.code, 200);

        crow::request getReq;
        getReq.url = "/api/cache/" + cacheKey;
        getReq.method = crow::HTTPMethod::Get;

        crow::response getRes;
        server.app.handle_full(getReq, getRes);
        EXPECT_EQ(getRes.code, 200);

        auto json = crow::json::load(getRes.body);
        EXPECT_TRUE(json);
        EXPECT_EQ(json["data"]["value"].s(), "e2e_cached_data");
    }

    // 3. Message Queue Publish, Poll & Ack
    std::string topic = "e2e.topic." + Infrastructure::Security::OpenSslCrypto::generateSecureToken(6);
    uint64_t queueMsgId = 0;
    {
        crow::request pubReq;
        pubReq.url = "/api/queue/publish";
        pubReq.method = crow::HTTPMethod::Post;
        pubReq.body = "{\"topic\":\"" + topic + "\",\"payload\":{\"orderId\":999}}";
        pubReq.add_header("Content-Type", "application/json");

        crow::response pubRes;
        server.app.handle_full(pubReq, pubRes);
        EXPECT_EQ(pubRes.code, 201);

        auto pubJson = crow::json::load(pubRes.body);
        EXPECT_TRUE(pubJson);
        queueMsgId = pubJson["data"]["id"].i();
        EXPECT_TRUE(queueMsgId > 0);

        crow::request pollReq;
        pollReq.url = "/api/queue/poll";
        pollReq.method = crow::HTTPMethod::Post;
        pollReq.body = "{\"topic\":\"" + topic + "\"}";
        pollReq.add_header("Content-Type", "application/json");

        crow::response pollRes;
        server.app.handle_full(pollReq, pollRes);
        EXPECT_EQ(pollRes.code, 200);

        crow::request ackReq;
        ackReq.url = "/api/queue/ack/" + std::to_string(queueMsgId);
        ackReq.method = crow::HTTPMethod::Post;

        crow::response ackRes;
        server.app.handle_full(ackReq, ackRes);
        EXPECT_EQ(ackRes.code, 200);
    }

    // 4. JSONB Documents Insert & Query
    std::string collection = "e2e_docs";
    std::string docId;
    {
        crow::request docReq;
        docReq.url = "/api/documents/" + collection;
        docReq.method = crow::HTTPMethod::Post;
        docReq.body = "{\"sku\":\"PROD-E2E-99\",\"price\":199.99,\"name\":\"High End Gadget\"}";
        docReq.add_header("Content-Type", "application/json");

        crow::response docRes;
        server.app.handle_full(docReq, docRes);
        EXPECT_EQ(docRes.code, 201);

        auto docJson = crow::json::load(docRes.body);
        EXPECT_TRUE(docJson);
        docId = docJson["data"]["id"].s();
        EXPECT_FALSE(docId.empty());

        // Query by JSON containment
        crow::request qReq;
        qReq.url = "/api/documents/" + collection + "/query";
        qReq.method = crow::HTTPMethod::Post;
        qReq.body = "{\"sku\":\"PROD-E2E-99\"}";
        qReq.add_header("Content-Type", "application/json");

        crow::response qRes;
        server.app.handle_full(qReq, qRes);
        EXPECT_EQ(qRes.code, 200);

        auto qJson = crow::json::load(qRes.body);
        EXPECT_TRUE(qJson);
        EXPECT_TRUE(qJson["data"].size() >= 1);
    }
}

// 6. Google OAuth2 & PKCE Endpoints
TEST_CASE("E2E::GoogleAuth", "GoogleAuthUrlAndPkceEndpoint") {
    auto& server = getTestServer();

    crow::request req;
    req.url = "/api/v1/auth/google/url";
    req.url_params = crow::query_string("?state=test-e2e-state-token");
    req.method = crow::HTTPMethod::Get;

    crow::response res;
    server.app.handle_full(req, res);
    EXPECT_EQ(res.code, 200);

    auto json = crow::json::load(res.body);
    EXPECT_TRUE(json);
    EXPECT_TRUE(json["success"].b());

    std::string authUrl = json["data"]["authUrl"].s();
    std::string state = json["data"]["state"].s();
    std::string codeVerifier = json["data"]["codeVerifier"].s();
    std::string codeChallenge = json["data"]["codeChallenge"].s();

    EXPECT_EQ(state, "test-e2e-state-token");
    EXPECT_EQ(codeVerifier.length(), 64);
    EXPECT_EQ(codeChallenge.length(), 43);
    EXPECT_TRUE(authUrl.find("code_challenge=") != std::string::npos);
    EXPECT_TRUE(authUrl.find("code_challenge_method=S256") != std::string::npos);
    EXPECT_TRUE(authUrl.find("state=test-e2e-state-token") != std::string::npos);
}

