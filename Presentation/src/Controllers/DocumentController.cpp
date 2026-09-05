#include "Presentation/Controllers/DocumentController.h"
#include "Presentation/Common/HttpResponseHelper.h"

namespace Presentation::Controllers {

DocumentController::DocumentController(std::shared_ptr<Application::UseCases::DocumentUseCases> useCases)
    : m_useCases(std::move(useCases)) {}

crow::response DocumentController::create(const std::string& collection, const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseCreateDocumentRequest(collection, req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_useCases->create(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response DocumentController::getById(const std::string& collection, const std::string& id) const {
    auto result = m_useCases->getById(collection, id);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response DocumentController::query(const std::string& collection, const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseQueryDocumentRequest(collection, req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_useCases->query(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response DocumentController::update(const std::string& collection, const std::string& id, const crow::request& req) const {
    if (req.body.empty()) {
        return Common::HttpResponseHelper::error(400, "Empty payload", {"Updated JSON document body is required."});
    }

    auto json = crow::json::load(req.body);
    if (!json) {
        return Common::HttpResponseHelper::error(400, "Invalid JSON syntax", {"Document update body must be valid JSON."});
    }

    auto result = m_useCases->update(collection, id, Application::DTOs::UpdateDocumentRequest{.data = req.body});
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response DocumentController::remove(const std::string& collection, const std::string& id) const {
    auto result = m_useCases->remove(collection, id);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

} // namespace Presentation::Controllers
