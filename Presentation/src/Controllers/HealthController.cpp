#include "Presentation/Controllers/HealthController.h"
#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/HealthDto.h"
#include "Presentation/Common/HttpResponseHelper.h"

namespace Presentation::Controllers {

HealthController::HealthController()
    : m_startTime(std::chrono::system_clock::now()) {}

crow::response HealthController::getHealth() const {
    const auto now = std::chrono::system_clock::now();
    const auto uptimeSec = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();

    Application::DTOs::HealthResponse health{
        .status = "healthy",
        .service = "CrowApi Multi-Module Clean Architecture",
        .version = "1.0.0",
        .uptimeSeconds = uptimeSec,
        .timestamp = Application::Common::currentTimestampIso8601()
    };

    auto apiResp = Application::Common::ApiResponse<Application::DTOs::HealthResponse>::ok(
        std::move(health),
        "Service is healthy and operating nominally",
        200
    );

    return Common::HttpResponseHelper::success(apiResp);
}

} // namespace Presentation::Controllers
