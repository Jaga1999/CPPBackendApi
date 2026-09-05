# CMakeLists.txt for Tests executable
cmake_minimum_required(VERSION 3.15)
project(Tests CXX)

add_executable(Tests
    "include/TestHarness.h"
    "main.cpp"
    "Unit/Domain/DomainEntitiesTest.cpp"
    "Unit/Application/ValidationTest.cpp"
    "Unit/Application/SecurityServiceTest.cpp"
    "Unit/Application/GoogleAuthPkceTest.cpp"
    "Unit/Application/UseCasesTest.cpp"
    "Unit/Infrastructure/EnvLoaderTest.cpp"
    "Unit/Infrastructure/OpenSslCryptoTest.cpp"
    "Unit/Infrastructure/RsaKeyManagerTest.cpp"
    "Unit/Infrastructure/JwtServiceTest.cpp"
    "Unit/Infrastructure/PostgresRepositoriesTest.cpp"
    "Unit/Presentation/HttpResponseHelperTest.cpp"
    "Unit/Presentation/LoggingMiddlewareTest.cpp"
    "Security/CyberAttacksTest.cpp"
    "Performance/PerformanceTests.cpp"
    "E2E/E2eApiTests.cpp"
)

target_compile_features(Tests PRIVATE cxx_std_20)

target_include_directories(Tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/Domain/include"
    "${CMAKE_SOURCE_DIR}/Application/include"
    "${CMAKE_SOURCE_DIR}/Infrastructure/include"
    "${CMAKE_SOURCE_DIR}/Presentation/include"
)

target_link_libraries(Tests PRIVATE
    Presentation
    Infrastructure
    Application
    Domain
    Crow::Crow
    libpqxx::pqxx
    OpenSSL::Crypto
)

if (MSVC)
    target_link_options(Tests PRIVATE /FORCE:MULTIPLE)
endif()

if (WIN32)
    add_custom_command(TARGET Tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/$<IF:$<CONFIG:Debug>,debug/bin,bin>"
            "$<TARGET_FILE_DIR:Tests>"
        COMMENT "Automatically copying all runtime dependency DLLs to Tests executable directory"
    )
endif()
