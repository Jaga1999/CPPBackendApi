#pragma once

#include "Application/UseCases/TodoUseCases.h"
#include <crow.h>
#include <memory>

namespace Presentation::Controllers {

class TodoController {
public:
    TodoController(
        std::shared_ptr<Application::UseCases::CreateTodoUseCase> createUseCase,
        std::shared_ptr<Application::UseCases::GetTodoByIdUseCase> getByIdUseCase,
        std::shared_ptr<Application::UseCases::ListTodosUseCase> listUseCase,
        std::shared_ptr<Application::UseCases::UpdateTodoUseCase> updateUseCase,
        std::shared_ptr<Application::UseCases::DeleteTodoUseCase> deleteUseCase
    );

    crow::response getAll(const crow::request& req) const;
    crow::response getById(uint64_t id) const;
    crow::response create(const crow::request& req) const;
    crow::response update(uint64_t id, const crow::request& req) const;
    crow::response remove(uint64_t id) const;

private:
    std::shared_ptr<Application::UseCases::CreateTodoUseCase> m_createUseCase;
    std::shared_ptr<Application::UseCases::GetTodoByIdUseCase> m_getByIdUseCase;
    std::shared_ptr<Application::UseCases::ListTodosUseCase> m_listUseCase;
    std::shared_ptr<Application::UseCases::UpdateTodoUseCase> m_updateUseCase;
    std::shared_ptr<Application::UseCases::DeleteTodoUseCase> m_deleteUseCase;
};

} // namespace Presentation::Controllers
