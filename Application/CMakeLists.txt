# CMakeLists.txt for Application library
cmake_minimum_required(VERSION 3.15)
project(Application CXX)

add_library(Application STATIC
    "include/Application/Common/ApiResponse.h"
    "include/Application/Common/OpenApiSpec.h"
    "src/Common/OpenApiSpec.cpp"
    "include/Application/DTOs/TodoDtos.h"
    "include/Application/DTOs/HealthDto.h"
    "include/Application/DTOs/CacheDtos.h"
    "include/Application/DTOs/QueueDtos.h"
    "include/Application/DTOs/DocumentDtos.h"
    "include/Application/DTOs/AuthDtos.h"
    "include/Application/Validation/InputValidator.h"
    "src/Validation/InputValidator.cpp"
    "include/Application/Validation/AuthInputValidator.h"
    "src/Validation/AuthInputValidator.cpp"
    "include/Application/Security/IKeyManager.h"
    "include/Application/Security/IJwtService.h"
    "include/Application/Security/IPasswordHasher.h"
    "include/Application/Security/ITokenGenerator.h"
    "include/Application/Security/AuthorizationService.h"
    "src/Security/AuthorizationService.cpp"
    "include/Application/UseCases/TodoUseCases.h"
    "src/UseCases/TodoUseCases.cpp"
    "include/Application/UseCases/CacheUseCases.h"
    "src/UseCases/CacheUseCases.cpp"
    "include/Application/UseCases/QueueUseCases.h"
    "src/UseCases/QueueUseCases.cpp"
    "include/Application/UseCases/DocumentUseCases.h"
    "src/UseCases/DocumentUseCases.cpp"
    "include/Application/UseCases/AuthUseCases.h"
    "src/UseCases/AuthUseCases.cpp"
    "include/Application/UseCases/SessionUseCases.h"
    "src/UseCases/SessionUseCases.cpp"
)

target_compile_features(Application PUBLIC cxx_std_20)

target_include_directories(Application PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(Application PUBLIC
    Domain
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    "include/Application/Common/ApiResponse.h"
    "include/Application/Common/OpenApiSpec.h"
    "src/Common/OpenApiSpec.cpp"
    "include/Application/DTOs/TodoDtos.h"
    "include/Application/DTOs/HealthDto.h"
    "include/Application/DTOs/CacheDtos.h"
    "include/Application/DTOs/QueueDtos.h"
    "include/Application/DTOs/DocumentDtos.h"
    "include/Application/DTOs/AuthDtos.h"
    "include/Application/Validation/InputValidator.h"
    "src/Validation/InputValidator.cpp"
    "include/Application/Validation/AuthInputValidator.h"
    "src/Validation/AuthInputValidator.cpp"
    "include/Application/Security/IKeyManager.h"
    "include/Application/Security/IJwtService.h"
    "include/Application/Security/IPasswordHasher.h"
    "include/Application/Security/ITokenGenerator.h"
    "include/Application/Security/AuthorizationService.h"
    "src/Security/AuthorizationService.cpp"
    "include/Application/UseCases/TodoUseCases.h"
    "src/UseCases/TodoUseCases.cpp"
    "include/Application/UseCases/CacheUseCases.h"
    "src/UseCases/CacheUseCases.cpp"
    "include/Application/UseCases/QueueUseCases.h"
    "src/UseCases/QueueUseCases.cpp"
    "include/Application/UseCases/DocumentUseCases.h"
    "src/UseCases/DocumentUseCases.cpp"
    "include/Application/UseCases/AuthUseCases.h"
    "src/UseCases/AuthUseCases.cpp"
    "include/Application/UseCases/SessionUseCases.h"
    "src/UseCases/SessionUseCases.cpp"
)
