#include "Presentation/Routes/Router.h"

namespace Presentation::Routes {

void Router::registerRoutes(
    AppType& app,
    const Controllers::HealthController& healthController,
    const Controllers::TodoController& todoController,
    const Controllers::CacheController& cacheController,
    const Controllers::QueueController& queueController,
    const Controllers::DocumentController& documentController,
    const Controllers::DocsController& docsController,
    const Controllers::AuthController& authController,
    const Controllers::SessionController& sessionController,
    const Controllers::AdminSessionController& adminSessionController
) {
    // ==========================================
    // Documentation & OpenAPI Endpoints
    // ==========================================
    CROW_ROUTE(app, "/api/openapi.json")
    ([&docsController]() {
        return docsController.getOpenApiJson();
    });

    CROW_ROUTE(app, "/docs")
    ([&docsController]() {
        return docsController.getSwaggerUi();
    });

    CROW_ROUTE(app, "/swagger")
    ([&docsController]() {
        return docsController.getSwaggerUi();
    });

    // ==========================================
    // JWKS Endpoint (RFC 7517 Public Key Discovery)
    // ==========================================
    CROW_ROUTE(app, "/.well-known/jwks.json")
        .methods(crow::HTTPMethod::Get)
    ([&authController]() {
        return authController.getJwks();
    });

    // ==========================================
    // Authentication Endpoints (JWT / Multi-Session)
    // ==========================================
    CROW_ROUTE(app, "/api/v1/auth/register")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.registerUser(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/login")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.login(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/refresh")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.refresh(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/logout")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.logout(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/logout-all")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.logoutAll(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/google/url")
        .methods(crow::HTTPMethod::Get)
    ([&authController](const crow::request& req) {
        return authController.getGoogleAuthUrl(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/google/callback")
        .methods(crow::HTTPMethod::Get)
    ([&authController](const crow::request& req) {
        return authController.googleCallback(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/google")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.googleLogin(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/set-password")
        .methods(crow::HTTPMethod::Post)
    ([&authController](const crow::request& req) {
        return authController.setPassword(req);
    });

    // ==========================================
    // User Multi-Session Endpoints
    // ==========================================
    CROW_ROUTE(app, "/api/v1/sessions")
        .methods(crow::HTTPMethod::Get)
    ([&sessionController](const crow::request& req) {
        return sessionController.getSessions(req);
    });

    CROW_ROUTE(app, "/api/v1/sessions/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([&sessionController](const crow::request& req, const std::string& sessionId) {
        return sessionController.revokeSession(req, sessionId);
    });

    // ==========================================
    // Admin Session Management Endpoints
    // ==========================================
    CROW_ROUTE(app, "/api/v1/admin/sessions")
        .methods(crow::HTTPMethod::Get)
    ([&adminSessionController](const crow::request& req) {
        return adminSessionController.getSessions(req);
    });

    CROW_ROUTE(app, "/api/v1/admin/sessions/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([&adminSessionController](const crow::request& req, const std::string& sessionId) {
        return adminSessionController.revokeSession(req, sessionId);
    });

    CROW_ROUTE(app, "/api/v1/admin/users/<string>/sessions/revoke-all")
        .methods(crow::HTTPMethod::Post)
    ([&adminSessionController](const crow::request& req, const std::string& userId) {
        return adminSessionController.revokeAllUserSessions(req, userId);
    });

    // ==========================================
    // Health Check Endpoints
    // ==========================================
    CROW_ROUTE(app, "/health")
    ([&healthController]() {
        return healthController.getHealth();
    });

    CROW_ROUTE(app, "/api/health")
    ([&healthController]() {
        return healthController.getHealth();
    });

    // ==========================================
    // 1. Relational SQL DB Endpoints (Todos)
    // ==========================================
    CROW_ROUTE(app, "/api/todos")
        .methods(crow::HTTPMethod::Get)
    ([&todoController](const crow::request& req) {
        return todoController.getAll(req);
    });

    CROW_ROUTE(app, "/api/todos")
        .methods(crow::HTTPMethod::Post)
    ([&todoController](const crow::request& req) {
        return todoController.create(req);
    });

    CROW_ROUTE(app, "/api/todos/<uint>")
        .methods(crow::HTTPMethod::Get)
    ([&todoController](uint64_t id) {
        return todoController.getById(id);
    });

    CROW_ROUTE(app, "/api/todos/<uint>")
        .methods(crow::HTTPMethod::Put)
    ([&todoController](const crow::request& req, uint64_t id) {
        return todoController.update(id, req);
    });

    CROW_ROUTE(app, "/api/todos/<uint>")
        .methods(crow::HTTPMethod::Delete)
    ([&todoController](uint64_t id) {
        return todoController.remove(id);
    });

    // ==========================================
    // 2. Redis Alternative Endpoints (Cache)
    // ==========================================
    CROW_ROUTE(app, "/api/cache/<string>")
        .methods(crow::HTTPMethod::Get)
    ([&cacheController](const std::string& key) {
        return cacheController.get(key);
    });

    CROW_ROUTE(app, "/api/cache")
        .methods(crow::HTTPMethod::Post)
    ([&cacheController](const crow::request& req) {
        return cacheController.set(req);
    });

    CROW_ROUTE(app, "/api/cache/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([&cacheController](const std::string& key) {
        return cacheController.remove(key);
    });

    CROW_ROUTE(app, "/api/cache/cleanup")
        .methods(crow::HTTPMethod::Post)
    ([&cacheController]() {
        return cacheController.cleanup();
    });

    // ==========================================
    // 3. Kafka Alternative Endpoints (Message Queue)
    // ==========================================
    CROW_ROUTE(app, "/api/queue/publish")
        .methods(crow::HTTPMethod::Post)
    ([&queueController](const crow::request& req) {
        return queueController.publish(req);
    });

    CROW_ROUTE(app, "/api/queue/poll")
        .methods(crow::HTTPMethod::Post)
    ([&queueController](const crow::request& req) {
        return queueController.poll(req);
    });

    CROW_ROUTE(app, "/api/queue/ack/<uint>")
        .methods(crow::HTTPMethod::Post)
    ([&queueController](uint64_t id) {
        return queueController.acknowledge(id);
    });

    CROW_ROUTE(app, "/api/queue/fail/<uint>")
        .methods(crow::HTTPMethod::Post)
    ([&queueController](uint64_t id) {
        return queueController.fail(id);
    });

    CROW_ROUTE(app, "/api/queue/metrics")
        .methods(crow::HTTPMethod::Get)
    ([&queueController]() {
        return queueController.getMetrics();
    });

    // ==========================================
    // 4. MongoDB Alternative Endpoints (Documents)
    // ==========================================
    CROW_ROUTE(app, "/api/documents/<string>")
        .methods(crow::HTTPMethod::Post)
    ([&documentController](const crow::request& req, const std::string& collection) {
        return documentController.create(collection, req);
    });

    CROW_ROUTE(app, "/api/documents/<string>/<string>")
        .methods(crow::HTTPMethod::Get)
    ([&documentController](const std::string& collection, const std::string& id) {
        return documentController.getById(collection, id);
    });

    CROW_ROUTE(app, "/api/documents/<string>/query")
        .methods(crow::HTTPMethod::Post)
    ([&documentController](const crow::request& req, const std::string& collection) {
        return documentController.query(collection, req);
    });

    CROW_ROUTE(app, "/api/documents/<string>/<string>")
        .methods(crow::HTTPMethod::Put)
    ([&documentController](const crow::request& req, const std::string& collection, const std::string& id) {
        return documentController.update(collection, id, req);
    });

    CROW_ROUTE(app, "/api/documents/<string>/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([&documentController](const std::string& collection, const std::string& id) {
        return documentController.remove(collection, id);
    });
}

} // namespace Presentation::Routes
