#include "Presentation/Controllers/DocsController.h"
#include "Application/Common/OpenApiSpec.h"

namespace Presentation::Controllers {

crow::response DocsController::getOpenApiJson() const {
    std::string jsonStr = Application::Common::OpenApiSpec::generateJson();
    crow::response res(200, jsonStr);
    res.set_header("Content-Type", "application/json");
    return res;
}

crow::response DocsController::getSwaggerUi() const {
    std::string htmlStr = Application::Common::OpenApiSpec::generateSwaggerHtml();
    crow::response res(200, htmlStr);
    res.set_header("Content-Type", "text/html; charset=utf-8");
    return res;
}

} // namespace Presentation::Controllers
