#include "Core.h"
#include "Infrastructure/Config/EnvLoader.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include "Infrastructure/Persistence/PostgresTodoRepository.h"
#include "Infrastructure/Persistence/PostgresCacheRepository.h"
#include "Infrastructure/Persistence/PostgresMessageQueueRepository.h"
#include "Infrastructure/Persistence/PostgresDocumentRepository.h"
#include "Infrastructure/Persistence/PostgresUserRepository.h"
#include "Infrastructure/Persistence/PostgresSessionRepository.h"
#include "Infrastructure/Persistence/PostgresAuditLogRepository.h"
#include "Infrastructure/Persistence/PostgresSessionRevocationListener.h"
#include "Infrastructure/Security/OpenSslCryptoAdapters.h"
#include "Infrastructure/Security/RsaKeyManager.h"
#include "Infrastructure/Security/JwtService.h"
#include "Infrastructure/Security/GoogleAuthService.h"

#include "Application/Security/AuthorizationService.h"
#include "Application/UseCases/TodoUseCases.h"
#include "Application/UseCases/CacheUseCases.h"
#include "Application/UseCases/QueueUseCases.h"
#include "Application/UseCases/DocumentUseCases.h"
#include "Application/UseCases/AuthUseCases.h"
#include "Application/UseCases/SessionUseCases.h"

#include "Presentation/Controllers/HealthController.h"
#include "Presentation/Controllers/TodoController.h"
#include "Presentation/Controllers/CacheController.h"
#include "Presentation/Controllers/QueueController.h"
#include "Presentation/Controllers/DocumentController.h"
#include "Presentation/Controllers/DocsController.h"
#include "Presentation/Controllers/AuthController.h"
#include "Presentation/Controllers/SessionController.h"
#include "Presentation/Controllers/AdminSessionController.h"

#include "Presentation/Middleware/LoggingMiddleware.h"
#include "Presentation/Routes/Router.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

int main(int argc, char* argv[]) {
    // 1. Load environment configuration from .env
    Infrastructure::Config::EnvLoader::load(".env");
    auto config = Infrastructure::Config::AppConfig::fromEnv();

    uint16_t port = config.serverPort;
    std::string logLevelStr = config.logLevel;

    // CLI arguments override .env settings if provided
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg.rfind("--port=", 0) == 0) {
            try {
                port = static_cast<uint16_t>(std::stoi(std::string(arg.substr(7))));
            } catch (...) {}
        } else if (arg.rfind("--log-level=", 0) == 0) {
            logLevelStr = std::string(arg.substr(12));
        } else if (i == 1) {
            try {
                port = static_cast<uint16_t>(std::stoi(argv[1]));
            } catch (...) {
                logLevelStr = argv[1];
            }
        } else if (i == 2) {
            logLevelStr = argv[2];
        }
    }

    auto appLogLevel = Presentation::Middleware::parseLogLevel(logLevelStr);
    Presentation::Middleware::LoggingMiddleware::setLogLevel(appLogLevel);

    std::cout << "=================================================================\n";
    std::cout << "  Crow Modern C++20 REST API Service                             \n";
    std::cout << "  Architecture : Clean Architecture (Multi-Module CMake)         \n";
    std::cout << "  Database     : PostgreSQL 18.6 (4-in-1 Multi-Paradigm Engine)   \n";
    std::cout << "                 1. Relational SQL DB     -> todos               \n";
    std::cout << "                 2. Redis KV Cache        -> cache_store         \n";
    std::cout << "                 3. Kafka Message Queue   -> message_queue       \n";
    std::cout << "                 4. MongoDB Document DB   -> documents (JSONB)   \n";
    std::cout << "  Port         : " << port << "\n";
    std::cout << "  Log Level    : " << logLevelStr << "\n";
    std::cout << "  Threads      : " << config.serverThreads << "\n";
    std::cout << "  Key ID       : " << config.jwtKeyId << "\n";
    std::cout << "=================================================================\n";

    // 2. PostgreSQL connection configuration from .env
    std::string connStr = config.toDbConnectionString();
    std::cout << "[PostgresDb] Connecting to: host=" << config.dbHost
              << " port=" << config.dbPort
              << " dbname=" << config.dbName
              << " (pool size: " << config.dbMaxPoolSize << ")...\n";
    auto db = std::make_shared<Infrastructure::Persistence::PostgresDb>(connStr, config.dbMaxPoolSize);

    // 2. Repositories (Infrastructure Layer)
    auto todoRepo = std::make_shared<Infrastructure::Persistence::PostgresTodoRepository>(db);
    auto cacheRepo = std::make_shared<Infrastructure::Persistence::PostgresCacheRepository>(db);
    auto queueRepo = std::make_shared<Infrastructure::Persistence::PostgresMessageQueueRepository>(db);
    auto docRepo = std::make_shared<Infrastructure::Persistence::PostgresDocumentRepository>(db);
    auto userRepo = std::make_shared<Infrastructure::Persistence::PostgresUserRepository>(db);
    auto sessionRepo = std::make_shared<Infrastructure::Persistence::PostgresSessionRepository>(db);
    auto auditLogRepo = std::make_shared<Infrastructure::Persistence::PostgresAuditLogRepository>(db);

    // 3. Security & Cryptography Infrastructure (OpenSSL 3.x RS256 & PBKDF2)
    auto passwordHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
    auto tokenGenerator = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
    auto keyManager = std::make_shared<Infrastructure::Security::RsaKeyManager>(config.jwtKeyId);

    auto jwtService = std::make_shared<Infrastructure::Security::JwtService>(keyManager, "crow-api-auth", "crow-api-clients");
    auto authService = std::make_shared<Application::Security::AuthorizationService>();

    // Start Cross-Instance Session Revocation Listener (PostgreSQL LISTEN/NOTIFY)
    auto revocationListener = std::make_unique<Infrastructure::Persistence::PostgresSessionRevocationListener>(
        connStr,
        [](std::string_view sessionId) {
            std::cout << "[SessionRevocationListener] Evicted cross-instance session: " << sessionId << std::endl;
        }
    );
    revocationListener->start();

    // 4. Use Cases (Application Layer)
    auto createTodoUseCase = std::make_shared<Application::UseCases::CreateTodoUseCase>(todoRepo);
    auto getTodoByIdUseCase = std::make_shared<Application::UseCases::GetTodoByIdUseCase>(todoRepo);
    auto listTodosUseCase = std::make_shared<Application::UseCases::ListTodosUseCase>(todoRepo);
    auto updateTodoUseCase = std::make_shared<Application::UseCases::UpdateTodoUseCase>(todoRepo);
    auto deleteTodoUseCase = std::make_shared<Application::UseCases::DeleteTodoUseCase>(todoRepo);

    auto cacheUseCases = std::make_shared<Application::UseCases::CacheUseCases>(cacheRepo);
    auto queueUseCases = std::make_shared<Application::UseCases::QueueUseCases>(queueRepo);
    auto docUseCases = std::make_shared<Application::UseCases::DocumentUseCases>(docRepo);

    auto googleAuthService = std::make_shared<Infrastructure::Security::GoogleAuthService>(
        config.googleClientId,
        config.googleClientSecret,
        config.googleRedirectUri
    );

    auto authUseCases = std::make_shared<Application::UseCases::AuthUseCases>(
        userRepo,
        sessionRepo,
        auditLogRepo,
        jwtService,
        passwordHasher,
        tokenGenerator,
        googleAuthService,
        cacheRepo
    );
    auto sessionUseCases = std::make_shared<Application::UseCases::SessionUseCases>(
        sessionRepo,
        userRepo,
        auditLogRepo,
        authService
    );

    // 5. Controllers (Presentation Layer)
    Presentation::Controllers::HealthController healthController;
    Presentation::Controllers::TodoController todoController(
        std::move(createTodoUseCase),
        std::move(getTodoByIdUseCase),
        std::move(listTodosUseCase),
        std::move(updateTodoUseCase),
        std::move(deleteTodoUseCase)
    );
    Presentation::Controllers::CacheController cacheController(std::move(cacheUseCases));
    Presentation::Controllers::QueueController queueController(std::move(queueUseCases));
    Presentation::Controllers::DocumentController documentController(std::move(docUseCases));
    Presentation::Controllers::DocsController docsController;

    Presentation::Controllers::AuthController authController(
        authUseCases,
        keyManager,
        jwtService,
        sessionRepo,
        userRepo
    );
    Presentation::Controllers::SessionController sessionController(
        sessionUseCases,
        jwtService,
        sessionRepo,
        userRepo
    );
    Presentation::Controllers::AdminSessionController adminSessionController(
        sessionUseCases,
        jwtService,
        sessionRepo,
        userRepo
    );

    // 6. Initialize Crow App with custom LoggingMiddleware
    Presentation::Routes::AppType app;
    app.loglevel(Presentation::Middleware::toCrowLogLevel(appLogLevel));

    // 7. Register all routes
    Presentation::Routes::Router::registerRoutes(
        app,
        healthController,
        todoController,
        cacheController,
        queueController,
        documentController,
        docsController,
        authController,
        sessionController,
        adminSessionController
    );

    std::cout << "\nAPI Documentation available at:\n";
    std::cout << "  Swagger UI   : http://localhost:" << port << "/docs\n";
    std::cout << "  OpenAPI 3.1  : http://localhost:" << port << "/api/openapi.json\n\n";

    std::cout << "Registered Endpoint Endpoints:\n";
    std::cout << "  [System]   GET    /health, /api/health\n";
    std::cout << "  [JWKS]     GET    /.well-known/jwks.json\n";
    std::cout << "  [Auth]     POST   /api/v1/auth/register\n";
    std::cout << "  [Auth]     POST   /api/v1/auth/login\n";
    std::cout << "  [Auth]     POST   /api/v1/auth/refresh\n";
    std::cout << "  [Auth]     POST   /api/v1/auth/logout\n";
    std::cout << "  [Auth]     POST   /api/v1/auth/logout-all\n";
    std::cout << "  [Sessions] GET    /api/v1/sessions\n";
    std::cout << "  [Sessions] DELETE /api/v1/sessions/<id>\n";
    std::cout << "  [Admin]    GET    /api/v1/admin/sessions\n";
    std::cout << "  [Admin]    DELETE /api/v1/admin/sessions/<id>\n";
    std::cout << "  [Admin]    POST   /api/v1/admin/users/<id>/sessions/revoke-all\n";
    std::cout << "  [SQL DB]   GET    /api/todos\n";
    std::cout << "  [SQL DB]   POST   /api/todos\n";
    std::cout << "  [SQL DB]   GET    /api/todos/<id>\n";
    std::cout << "  [SQL DB]   PUT    /api/todos/<id>\n";
    std::cout << "  [SQL DB]   DELETE /api/todos/<id>\n";
    std::cout << "  [Redis]    GET    /api/cache/<key>\n";
    std::cout << "  [Redis]    POST   /api/cache\n";
    std::cout << "  [Redis]    DELETE /api/cache/<key>\n";
    std::cout << "  [Redis]    POST   /api/cache/cleanup\n";
    std::cout << "  [Kafka]    POST   /api/queue/publish\n";
    std::cout << "  [Kafka]    POST   /api/queue/poll\n";
    std::cout << "  [Kafka]    POST   /api/queue/ack/<id>\n";
    std::cout << "  [Kafka]    POST   /api/queue/fail/<id>\n";
    std::cout << "  [Kafka]    GET    /api/queue/metrics\n";
    std::cout << "  [MongoDB]  POST   /api/documents/<col>\n";
    std::cout << "  [MongoDB]  GET    /api/documents/<col>/<id>\n";
    std::cout << "  [MongoDB]  POST   /api/documents/<col>/query\n";
    std::cout << "  [MongoDB]  PUT    /api/documents/<col>/<id>\n";
    std::cout << "  [MongoDB]  DELETE /api/documents/<col>/<id>\n";
    std::cout << "=================================================================\n";

    app.concurrency(config.serverThreads).port(port).multithreaded().run();

    return 0;
}
