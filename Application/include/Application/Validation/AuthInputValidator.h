#pragma once

#include "Application/DTOs/AuthDtos.h"
#include "Domain/Common/Result.h"
#include <string_view>

namespace Application::Validation {

class AuthInputValidator {
public:
    static Domain::Common::Result<void> validateRegister(const DTOs::RegisterRequest& req);
    static Domain::Common::Result<void> validateLogin(const DTOs::LoginRequest& req);
    static Domain::Common::Result<void> validateRefreshToken(const DTOs::RefreshTokenRequest& req);
};

} // namespace Application::Validation
