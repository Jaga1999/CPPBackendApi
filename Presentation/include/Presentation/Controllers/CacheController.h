#pragma once

#include "Application/UseCases/CacheUseCases.h"
#include <crow.h>
#include <memory>
#include <string>

namespace Presentation::Controllers {

class CacheController {
public:
    explicit CacheController(std::shared_ptr<Application::UseCases::CacheUseCases> useCases);

    crow::response get(const std::string& key) const;
    crow::response set(const crow::request& req) const;
    crow::response remove(const std::string& key) const;
    crow::response cleanup() const;

private:
    std::shared_ptr<Application::UseCases::CacheUseCases> m_useCases;
};

} // namespace Presentation::Controllers
