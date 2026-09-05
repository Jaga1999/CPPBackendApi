#pragma once

#include <string>

namespace Application::Common {

class OpenApiSpec {
public:
    static std::string generateJson();
    static std::string generateSwaggerHtml();
};

} // namespace Application::Common
