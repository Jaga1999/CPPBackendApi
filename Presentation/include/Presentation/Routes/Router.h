#pragma once

#include "Presentation/Controllers/AdminSessionController.h"
#include "Presentation/Controllers/AuthController.h"
#include "Presentation/Controllers/CacheController.h"
#include "Presentation/Controllers/DocsController.h"
#include "Presentation/Controllers/DocumentController.h"
#include "Presentation/Controllers/HealthController.h"
#include "Presentation/Controllers/QueueController.h"
#include "Presentation/Controllers/SessionController.h"
#include "Presentation/Controllers/TodoController.h"
#include "Presentation/Middleware/LoggingMiddleware.h"
#include <crow.h>

namespace Presentation::Routes {

using AppType = crow::App<Middleware::LoggingMiddleware>;

class Router {
public:
    static void registerRoutes(
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
    );
};

} // namespace Presentation::Routes
