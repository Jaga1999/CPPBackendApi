#include "Domain/Entities/Todo.h"

namespace Domain::Entities {

Common::Result<void, Common::DomainError> Todo::validate(std::string_view title) {
    if (title.empty()) {
        return Common::Result<void, Common::DomainError>::err(
            Common::DomainError{
                .message = "Domain validation failed",
                .statusCode = 400,
                .details = {"Field 'title' cannot be empty"}
            }
        );
    }

    if (title.length() > 255) {
        return Common::Result<void, Common::DomainError>::err(
            Common::DomainError{
                .message = "Domain validation failed",
                .statusCode = 400,
                .details = {"Field 'title' cannot exceed 255 characters"}
            }
        );
    }

    return Common::Result<void, Common::DomainError>::ok();
}

} // namespace Domain::Entities
