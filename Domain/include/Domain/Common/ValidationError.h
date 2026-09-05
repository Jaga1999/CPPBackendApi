#pragma once

#include <string>
#include <vector>

namespace Domain::Common {

struct ValidationError {
    std::string field;
    std::string message;
};

struct DomainError {
    std::string message;
    int statusCode{400};
    std::vector<std::string> details{};
};

} // namespace Domain::Common
