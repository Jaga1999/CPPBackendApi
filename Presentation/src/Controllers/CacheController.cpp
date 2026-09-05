#include "Presentation/Controllers/CacheController.h"
#include "Presentation/Common/HttpResponseHelper.h"

namespace Presentation::Controllers {

CacheController::CacheController(std::shared_ptr<Application::UseCases::CacheUseCases> useCases)
    : m_useCases(std::move(useCases)) {}

crow::response CacheController::get(const std::string& key) const {
    auto result = m_useCases->get(key);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }
    return Common::HttpResponseHelper::success(result.value());
}

crow::response CacheController::set(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseSetCacheRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_useCases->set(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }
    return Common::HttpResponseHelper::success(result.value());
}

crow::response CacheController::remove(const std::string& key) const {
    auto result = m_useCases->remove(key);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }
    return Common::HttpResponseHelper::success(result.value());
}

crow::response CacheController::cleanup() const {
    auto resp = m_useCases->cleanup();
    return Common::HttpResponseHelper::success(resp);
}

} // namespace Presentation::Controllers
