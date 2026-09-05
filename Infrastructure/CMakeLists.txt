# CMakeLists.txt for Infrastructure library
cmake_minimum_required(VERSION 3.15)
project(Infrastructure CXX)

add_library(Infrastructure STATIC
    "include/Infrastructure/Persistence/InMemoryTodoRepository.h"
    "src/Persistence/InMemoryTodoRepository.cpp"
    "include/Infrastructure/Persistence/PostgresDb.h"
    "src/Persistence/PostgresDb.cpp"
    "include/Infrastructure/Persistence/PostgresTodoRepository.h"
    "src/Persistence/PostgresTodoRepository.cpp"
    "include/Infrastructure/Persistence/PostgresCacheRepository.h"
    "src/Persistence/PostgresCacheRepository.cpp"
    "include/Infrastructure/Persistence/PostgresMessageQueueRepository.h"
    "src/Persistence/PostgresMessageQueueRepository.cpp"
    "include/Infrastructure/Persistence/PostgresDocumentRepository.h"
    "src/Persistence/PostgresDocumentRepository.cpp"
    "include/Infrastructure/Security/OpenSslCrypto.h"
    "src/Security/OpenSslCrypto.cpp"
    "include/Infrastructure/Security/RsaKeyManager.h"
    "src/Security/RsaKeyManager.cpp"
    "include/Infrastructure/Security/JwtService.h"
    "src/Security/JwtService.cpp"
    "include/Infrastructure/Persistence/PostgresUserRepository.h"
    "src/Persistence/PostgresUserRepository.cpp"
    "include/Infrastructure/Persistence/PostgresSessionRepository.h"
    "src/Persistence/PostgresSessionRepository.cpp"
    "include/Infrastructure/Persistence/PostgresAuditLogRepository.h"
    "src/Persistence/PostgresAuditLogRepository.cpp"
    "include/Infrastructure/Persistence/PostgresSessionRevocationListener.h"
    "src/Persistence/PostgresSessionRevocationListener.cpp"
    "include/Infrastructure/Config/EnvLoader.h"
    "src/Config/EnvLoader.cpp"
    "include/Infrastructure/Security/GoogleAuthService.h"
    "src/Security/GoogleAuthService.cpp"
)

target_compile_features(Infrastructure PUBLIC cxx_std_20)

target_include_directories(Infrastructure PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(Infrastructure PUBLIC
    Domain
    Application
    libpqxx::pqxx
    OpenSSL::Crypto
)

if (WIN32)
    target_link_libraries(Infrastructure PUBLIC winhttp)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    "include/Infrastructure/Persistence/InMemoryTodoRepository.h"
    "src/Persistence/InMemoryTodoRepository.cpp"
    "include/Infrastructure/Persistence/PostgresDb.h"
    "src/Persistence/PostgresDb.cpp"
    "include/Infrastructure/Persistence/PostgresTodoRepository.h"
    "src/Persistence/PostgresTodoRepository.cpp"
    "include/Infrastructure/Persistence/PostgresCacheRepository.h"
    "src/Persistence/PostgresCacheRepository.cpp"
    "include/Infrastructure/Persistence/PostgresMessageQueueRepository.h"
    "src/Persistence/PostgresMessageQueueRepository.cpp"
    "include/Infrastructure/Persistence/PostgresDocumentRepository.h"
    "src/Persistence/PostgresDocumentRepository.cpp"
    "include/Infrastructure/Security/OpenSslCrypto.h"
    "src/Security/OpenSslCrypto.cpp"
    "include/Infrastructure/Security/RsaKeyManager.h"
    "src/Security/RsaKeyManager.cpp"
    "include/Infrastructure/Security/JwtService.h"
    "src/Security/JwtService.cpp"
    "include/Infrastructure/Persistence/PostgresUserRepository.h"
    "src/Persistence/PostgresUserRepository.cpp"
    "include/Infrastructure/Persistence/PostgresSessionRepository.h"
    "src/Persistence/PostgresSessionRepository.cpp"
    "include/Infrastructure/Persistence/PostgresAuditLogRepository.h"
    "src/Persistence/PostgresAuditLogRepository.cpp"
    "include/Infrastructure/Persistence/PostgresSessionRevocationListener.h"
    "src/Persistence/PostgresSessionRevocationListener.cpp"
    "include/Infrastructure/Config/EnvLoader.h"
    "src/Config/EnvLoader.cpp"
)
