#pragma once

#include "Application/DTOs/TodoDtos.h"
#include "Domain/Common/Result.h"
#include <string>
#include <string_view>
#include <vector>

namespace Application::Validation {

class InputValidator {
public:
    static Domain::Common::Result<void, Domain::Common::DomainError> validateCreate(
        const DTOs::CreateTodoRequest& request
    );

    static Domain::Common::Result<void, Domain::Common::DomainError> validateUpdate(
        const DTOs::UpdateTodoRequest& request
    );

private:
    static bool isAllWhitespace(std::string_view str);
};

} // namespace Application::Validation
