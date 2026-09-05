#include "Presentation/Controllers/QueueController.h"
#include "Presentation/Common/HttpResponseHelper.h"

namespace Presentation::Controllers {

QueueController::QueueController(std::shared_ptr<Application::UseCases::QueueUseCases> useCases)
    : m_useCases(std::move(useCases)) {}

crow::response QueueController::publish(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parsePublishMessageRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_useCases->publish(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response QueueController::poll(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parsePollMessageRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_useCases->poll(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response QueueController::acknowledge(uint64_t id) const {
    auto result = m_useCases->acknowledge(id);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response QueueController::fail(uint64_t id) const {
    auto result = m_useCases->fail(id);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response QueueController::getMetrics() const {
    auto resp = m_useCases->getMetrics();
    return Common::HttpResponseHelper::success(resp);
}

} // namespace Presentation::Controllers
