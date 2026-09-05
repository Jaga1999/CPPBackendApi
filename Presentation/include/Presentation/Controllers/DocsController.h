#pragma once

#include <crow.h>

namespace Presentation::Controllers {

class DocsController {
public:
    crow::response getOpenApiJson() const;
    crow::response getSwaggerUi() const;
};

} // namespace Presentation::Controllers
