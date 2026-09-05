# CMakeLists.txt for Presentation library
cmake_minimum_required(VERSION 3.15)
project(Presentation CXX)

add_library(Presentation STATIC
    "include/Presentation/Common/HttpResponseHelper.h"
    "src/Common/HttpResponseHelper.cpp"
    "include/Presentation/Middleware/LoggingMiddleware.h"
    "include/Presentation/Middleware/AuthContext.h"
    "include/Presentation/Controllers/HealthController.h"
    "src/Controllers/HealthController.cpp"
    "include/Presentation/Controllers/TodoController.h"
    "src/Controllers/TodoController.cpp"
    "include/Presentation/Controllers/CacheController.h"
    "src/Controllers/CacheController.cpp"
    "include/Presentation/Controllers/QueueController.h"
    "src/Controllers/QueueController.cpp"
    "include/Presentation/Controllers/DocumentController.h"
    "src/Controllers/DocumentController.cpp"
    "include/Presentation/Controllers/DocsController.h"
    "src/Controllers/DocsController.cpp"
    "include/Presentation/Controllers/AuthController.h"
    "src/Controllers/AuthController.cpp"
    "include/Presentation/Controllers/SessionController.h"
    "src/Controllers/SessionController.cpp"
    "include/Presentation/Controllers/AdminSessionController.h"
    "src/Controllers/AdminSessionController.cpp"
    "include/Presentation/Routes/Router.h"
    "src/Routes/Router.cpp"
)

target_compile_features(Presentation PUBLIC cxx_std_20)

target_compile_definitions(Presentation PUBLIC
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_include_directories(Presentation PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(Presentation PUBLIC
    Application
    Crow::Crow
    ws2_32
    mswsock
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    "include/Presentation/Common/HttpResponseHelper.h"
    "src/Common/HttpResponseHelper.cpp"
    "include/Presentation/Middleware/LoggingMiddleware.h"
    "include/Presentation/Middleware/AuthContext.h"
    "include/Presentation/Controllers/HealthController.h"
    "src/Controllers/HealthController.cpp"
    "include/Presentation/Controllers/TodoController.h"
    "src/Controllers/TodoController.cpp"
    "include/Presentation/Controllers/CacheController.h"
    "src/Controllers/CacheController.cpp"
    "include/Presentation/Controllers/QueueController.h"
    "src/Controllers/QueueController.cpp"
    "include/Presentation/Controllers/DocumentController.h"
    "src/Controllers/DocumentController.cpp"
    "include/Presentation/Controllers/DocsController.h"
    "src/Controllers/DocsController.cpp"
    "include/Presentation/Controllers/AuthController.h"
    "src/Controllers/AuthController.cpp"
    "include/Presentation/Controllers/SessionController.h"
    "src/Controllers/SessionController.cpp"
    "include/Presentation/Controllers/AdminSessionController.h"
    "src/Controllers/AdminSessionController.cpp"
    "include/Presentation/Routes/Router.h"
    "src/Routes/Router.cpp"
)
