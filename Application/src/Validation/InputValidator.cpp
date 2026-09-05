#include "Application/Validation/InputValidator.h"
#include <algorithm>
#include <cctype>

namespace Application::Validation {

bool InputValidator::isAllWhitespace(std::string_view str) {
    return std::ranges::all_of(str, [](unsigned char ch) {
        return std::isspace(ch);
    });
}

Domain::Common::Result<void, Domain::Common::DomainError> InputValidator::validateCreate(
    const DTOs::CreateTodoRequest& request
) {
    std::vector<std::string> errors;

    if (request.title.empty()) {
        errors.push_back("Field 'title' is required and cannot be empty.");
    } else if (isAllWhitespace(request.title)) {
        errors.push_back("Field 'title' cannot consist solely of whitespace.");
    } else if (request.title.length() > 255) {
        errors.push_back("Field 'title' must not exceed 255 characters.");
    }

    if (request.description.length() > 1000) {
        errors.push_back("Field 'description' must not exceed 1000 characters.");
    }

    if (!errors.empty()) {
        return Domain::Common::Result<void, Domain::Common::DomainError>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for CreateTodoRequest",
                .statusCode = 400,
                .details = std::move(errors)
            }
        );
    }

    return Domain::Common::Result<void, Domain::Common::DomainError>::ok();
}

Domain::Common::Result<void, Domain::Common::DomainError> InputValidator::validateUpdate(
    const DTOs::UpdateTodoRequest& request
) {
    std::vector<std::string> errors;

    if (!request.title.has_value() && !request.description.has_value() && !request.completed.has_value()) {
        errors.push_back("At least one field ('title', 'description', or 'completed') must be supplied for update.");
    }

    if (request.title.has_value()) {
        if (request.title->empty()) {
            errors.push_back("Field 'title' cannot be empty.");
        } else if (isAllWhitespace(*request.title)) {
            errors.push_back("Field 'title' cannot consist solely of whitespace.");
        } else if (request.title->length() > 255) {
            errors.push_back("Field 'title' must not exceed 255 characters.");
        }
    }

    if (request.description.has_value() && request.description->length() > 1000) {
        errors.push_back("Field 'description' must not exceed 1000 characters.");
    }

    if (!errors.empty()) {
        return Domain::Common::Result<void, Domain::Common::DomainError>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for UpdateTodoRequest",
                .statusCode = 400,
                .details = std::move(errors)
            }
        );
    }

    return Domain::Common::Result<void, Domain::Common::DomainError>::ok();
}

} // namespace Application::Validation
