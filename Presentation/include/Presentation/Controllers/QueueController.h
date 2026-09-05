#pragma once

#include "Application/UseCases/QueueUseCases.h"
#include <crow.h>
#include <memory>

namespace Presentation::Controllers {

class QueueController {
public:
    explicit QueueController(std::shared_ptr<Application::UseCases::QueueUseCases> useCases);

    crow::response publish(const crow::request& req) const;
    crow::response poll(const crow::request& req) const;
    crow::response acknowledge(uint64_t id) const;
    crow::response fail(uint64_t id) const;
    crow::response getMetrics() const;

private:
    std::shared_ptr<Application::UseCases::QueueUseCases> m_useCases;
};

} // namespace Presentation::Controllers
