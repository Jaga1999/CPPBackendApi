#pragma once

#include <cstdint>
#include <string>

namespace Application::DTOs {

struct HealthResponse {
    std::string status{"healthy"};
    std::string service{"CrowApi Multi-Module Clean Architecture"};
    std::string version{"1.0.0"};
    int64_t uptimeSeconds{0};
    std::string timestamp;
};

} // namespace Application::DTOs
