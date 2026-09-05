#pragma once

#include <chrono>
#include <crow.h>

namespace Presentation::Controllers {

class HealthController {
public:
    HealthController();

    crow::response getHealth() const;

private:
    std::chrono::system_clock::time_point m_startTime;
};

} // namespace Presentation::Controllers
