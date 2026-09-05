#include "Presentation/Controllers/TodoController.h"
#include "Presentation/Common/HttpResponseHelper.h"
#include <string_view>

namespace Presentation::Controllers {

TodoController::TodoController(
    std::shared_ptr<Application::UseCases::CreateTodoUseCase> createUseCase,
    std::shared_ptr<Application::UseCases::GetTodoByIdUseCase> getByIdUseCase,
    std::shared_ptr<Application::UseCases::ListTodosUseCase> listUseCase,
    std::shared_ptr<Application::UseCases::UpdateTodoUseCase> updateUseCase,
    std::shared_ptr<Application::UseCases::DeleteTodoUseCase> deleteUseCase
)
    : m_createUseCase(std::move(createUseCase)),
      m_getByIdUseCase(std::move(getByIdUseCase)),
      m_listUseCase(std::move(listUseCase)),
      m_updateUseCase(std::move(updateUseCase)),
      m_deleteUseCase(std::move(deleteUseCase)) {}

crow::response TodoController::getAll(const crow::request& req) const {
    std::optional<bool> completedFilter = std::nullopt;
    const char* completedParam = req.url_params.get("completed");
    if (completedParam != nullptr) {
        std::string_view val(completedParam);
        if (val == "true" || val == "1") {
            completedFilter = true;
        } else if (val == "false" || val == "0") {
            completedFilter = false;
        } else {
            return Common::HttpResponseHelper::error(
                400,
                "Invalid query parameter",
                {"Query parameter 'completed' must be either 'true' or 'false'."}
            );
        }
    }

    auto apiResp = m_listUseCase->execute(completedFilter);
    return Common::HttpResponseHelper::success(apiResp);
}

crow::response TodoController::getById(uint64_t id) const {
    if (id == 0) {
        return Common::HttpResponseHelper::error(
            400,
            "Invalid ID parameter",
            {"ID must be a positive non-zero integer."}
        );
    }

    auto result = m_getByIdUseCase->execute(id);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response TodoController::create(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseCreateTodoRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_createUseCase->execute(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response TodoController::update(uint64_t id, const crow::request& req) const {
    if (id == 0) {
        return Common::HttpResponseHelper::error(
            400,
            "Invalid ID parameter",
            {"ID must be a positive non-zero integer."}
        );
    }

    auto parsed = Common::HttpResponseHelper::parseUpdateTodoRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_updateUseCase->execute(id, parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response TodoController::remove(uint64_t id) const {
    if (id == 0) {
        return Common::HttpResponseHelper::error(
            400,
            "Invalid ID parameter",
            {"ID must be a positive non-zero integer."}
        );
    }

    auto result = m_deleteUseCase->execute(id);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

} // namespace Presentation::Controllers
