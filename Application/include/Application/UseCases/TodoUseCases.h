#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/TodoDtos.h"
#include "Domain/Common/Result.h"
#include "Domain/Repositories/ITodoRepository.h"
#include <memory>
#include <optional>
#include <vector>

namespace Application::UseCases {

class CreateTodoUseCase {
public:
    explicit CreateTodoUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository);
    Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>> execute(DTOs::CreateTodoRequest request);

private:
    std::shared_ptr<Domain::Repositories::ITodoRepository> m_repository;
};

class GetTodoByIdUseCase {
public:
    explicit GetTodoByIdUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository);
    Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>> execute(uint64_t id);

private:
    std::shared_ptr<Domain::Repositories::ITodoRepository> m_repository;
};

class ListTodosUseCase {
public:
    explicit ListTodosUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository);
    Common::ApiResponse<std::vector<DTOs::TodoResponse>> execute(std::optional<bool> completedFilter = std::nullopt);

private:
    std::shared_ptr<Domain::Repositories::ITodoRepository> m_repository;
};

class UpdateTodoUseCase {
public:
    explicit UpdateTodoUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository);
    Domain::Common::Result<Common::ApiResponse<DTOs::TodoResponse>> execute(uint64_t id, DTOs::UpdateTodoRequest request);

private:
    std::shared_ptr<Domain::Repositories::ITodoRepository> m_repository;
};

class DeleteTodoUseCase {
public:
    explicit DeleteTodoUseCase(std::shared_ptr<Domain::Repositories::ITodoRepository> repository);
    Domain::Common::Result<Common::ApiResponse<void>> execute(uint64_t id);

private:
    std::shared_ptr<Domain::Repositories::ITodoRepository> m_repository;
};

} // namespace Application::UseCases
