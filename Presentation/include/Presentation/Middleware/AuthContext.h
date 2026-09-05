#pragma once

#include <string>

namespace Presentation::Middleware {

struct AuthenticatedUser {
    std::string id;
    std::string email;
    std::string role;
    std::string sessionId;
    std::string jti;
};

} // namespace Presentation::Middleware
