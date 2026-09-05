#pragma once

#include "Application/UseCases/DocumentUseCases.h"
#include <crow.h>
#include <memory>
#include <string>

namespace Presentation::Controllers {

class DocumentController {
public:
    explicit DocumentController(std::shared_ptr<Application::UseCases::DocumentUseCases> useCases);

    crow::response create(const std::string& collection, const crow::request& req) const;
    crow::response getById(const std::string& collection, const std::string& id) const;
    crow::response query(const std::string& collection, const crow::request& req) const;
    crow::response update(const std::string& collection, const std::string& id, const crow::request& req) const;
    crow::response remove(const std::string& collection, const std::string& id) const;

private:
    std::shared_ptr<Application::UseCases::DocumentUseCases> m_useCases;
};

} // namespace Presentation::Controllers
