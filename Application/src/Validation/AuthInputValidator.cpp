#include "Application/Validation/AuthInputValidator.h"
#include <vector>

namespace Application::Validation {

namespace {

bool isAllWhitespace(std::string_view s) {
    return s.empty() || s.find_first_not_of(" \t\n\r") == std::string_view::npos;
}

bool isValidEmail(std::string_view email) {
    if (email.size() < 3 || email.size() > 255) return false;
    size_t at = email.find('@');
    if (at == std::string_view::npos || at == 0 || at == email.size() - 1) return false;
    size_t dot = email.find('.', at + 1);
    if (dot == std::string_view::npos || dot == at + 1 || dot == email.size() - 1) return false;
    return true;
}

} // anonymous namespace

Domain::Common::Result<void> AuthInputValidator::validateRegister(const DTOs::RegisterRequest& req) {
    std::vector<std::string> errors;

    if (isAllWhitespace(req.email)) {
        errors.push_back("Field 'email' is required and cannot be empty.");
    } else if (!isValidEmail(req.email)) {
        errors.push_back("Field 'email' must be a valid email address (e.g. user@example.com).");
    }

    if (req.password.empty()) {
        errors.push_back("Field 'password' is required.");
    } else if (req.password.size() < 8) {
        errors.push_back("Field 'password' must be at least 8 characters long.");
    } else if (req.password.size() > 128) {
        errors.push_back("Field 'password' must not exceed 128 characters.");
    }

    if (!req.role.empty() && req.role != "user" && req.role != "admin") {
        errors.push_back("Field 'role' must be either 'user' or 'admin'.");
    }

    if (!errors.empty()) {
        return Domain::Common::Result<void>::err(Domain::Common::DomainError{
            .message = "Validation failed for RegisterRequest",
            .statusCode = 400,
            .details = std::move(errors)
        });
    }

    return Domain::Common::Result<void>::ok();
}

Domain::Common::Result<void> AuthInputValidator::validateLogin(const DTOs::LoginRequest& req) {
    std::vector<std::string> errors;

    if (isAllWhitespace(req.email)) {
        errors.push_back("Field 'email' is required.");
    }
    if (req.password.empty()) {
        errors.push_back("Field 'password' is required.");
    }

    if (!errors.empty()) {
        return Domain::Common::Result<void>::err(Domain::Common::DomainError{
            .message = "Validation failed for LoginRequest",
            .statusCode = 400,
            .details = std::move(errors)
        });
    }

    return Domain::Common::Result<void>::ok();
}

Domain::Common::Result<void> AuthInputValidator::validateRefreshToken(const DTOs::RefreshTokenRequest& req) {
    if (isAllWhitespace(req.refreshToken)) {
        return Domain::Common::Result<void>::err(Domain::Common::DomainError{
            .message = "Validation failed for RefreshTokenRequest",
            .statusCode = 400,
            .details = {"Field 'refreshToken' is required."}
        });
    }
    return Domain::Common::Result<void>::ok();
}

} // namespace Application::Validation
