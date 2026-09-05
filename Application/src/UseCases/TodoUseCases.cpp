#include "Application/UseCases/TodoUseCases.h"
#include "Application/Validation/InputValidator.h"
#include <chrono>
#include <format>
#include <ranges>

namespace Application::UseCases {

CreateTodoUseCase::CreateTodoUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>> CreateTodoUseCase::execute(
    DTOs::CreateTodoRequest request
) {
    auto valResult = Validation::InputValidator::validateCreate(request);
    if (valResult.isErr()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::err(valResult.error());
    }

    const auto now = std::chrono::system_clock::now();
    Domain::Entities::Todo domainTodo{
        .id = 0,
        .title = std::move(request.title),
        .description = std::move(request.description),
        .completed = false,
        .createdAt = now,
        .updatedAt = now
    };

    auto saved = m_repository->save(std::move(domainTodo));
    auto responseDto = DTOs::TodoResponse::fromDomain(saved);

    return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::ok(
        Common::ApiResponse<DTOs::TodoResponse>::ok(
            std::move(responseDto),
            "Todo created successfully",
            201
        )
    );
}

GetTodoByIdUseCase::GetTodoByIdUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>> GetTodoByIdUseCase::execute(uint64_t id) {
    auto found = m_repository->findById(id);
    if (!found.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::err(
            Domain::Common::DomainError{
                .message = std::format("Todo with id {} was not found", id),
                .statusCode = 404,
                .details = {std::format("No Todo entity matches the requested id: {}", id)}
            }
        );
    }

    auto responseDto = DTOs::TodoResponse::fromDomain(*found);
    return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::ok(
        Common::ApiResponse<DTOs::TodoResponse>::ok(
            std::move(responseDto),
            "Todo retrieved successfully",
            200
        )
    );
}

ListTodosUseCase::ListTodosUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository)
    : m_repository(std::move(repository)) {}

Common::ApiResponse<std::vector<DTOs::TodoResponse>> ListTodosUseCase::execute(
    std::optional<bool> completedFilter
) {
    auto domainTodos = m_repository->findAll();

    std::vector<DTOs::TodoResponse> dtoList;

    // C++20 Ranges pipeline
    auto filtered = domainTodos | std::views::filter([completedFilter](const Domain::Entities::Todo& item) {
        return !completedFilter.has_value() || item.completed == *completedFilter;
    });

    for (const auto& item : filtered) {
        dtoList.push_back(DTOs::TodoResponse::fromDomain(item));
    }

    std::string msg = completedFilter.has_value()
        ? std::format("Retrieved {} todos with completed = {}", dtoList.size(), *completedFilter)
        : std::format("Retrieved all {} todos", dtoList.size());

    return Common::ApiResponse<std::vector<DTOs::TodoResponse>>::ok(
        std::move(dtoList),
        std::move(msg),
        200
    );
}

UpdateTodoUseCase::UpdateTodoUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>> UpdateTodoUseCase::execute(
    uint64_t id,
    DTOs::UpdateTodoRequest request
) {
    auto valResult = Validation::InputValidator::validateUpdate(request);
    if (valResult.isErr()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::err(valResult.error());
    }

    std::optional<std::string_view> titleOpt = request.title.has_value()
        ? std::optional<std::string_view>{*request.title}
        : std::nullopt;

    std::optional<std::string_view> descOpt = request.description.has_value()
        ? std::optional<std::string_view>{*request.description}
        : std::nullopt;

    auto updated = m_repository->update(id, titleOpt, descOpt, request.completed);
    if (!updated.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::err(
            Domain::Common::DomainError{
                .message = std::format("Todo with id {} was not found for update", id),
                .statusCode = 404,
                .details = {std::format("Cannot update non-existent Todo with id: {}", id)}
            }
        );
    }

    auto responseDto = DTOs::TodoResponse::fromDomain(*updated);
    return Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>>::ok(
        Common::ApiResponse<DTOs::TodoResponse>::ok(
            std::move(responseDto),
            "Todo updated successfully",
            200
        )
    );
}

DeleteTodoUseCase::DeleteTodoUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<void>> DeleteTodoUseCase::execute(uint64_t id) {
    bool removed = m_repository->remove(id);
    if (!removed) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = std::format("Todo with id {} was not found for deletion", id),
                .statusCode = 404,
                .details = {std::format("Cannot delete non-existent Todo with id: {}", id)}
            }
        );
    }

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok(
            std::format("Todo with id {} deleted successfully", id),
            200
        )
    );
}

} // namespace Application::UseCases
