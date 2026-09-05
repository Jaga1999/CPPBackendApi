# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CrowApi" CXX)

# Global Modern C++20 standard (Strict ISO C++20, no vendor extensions)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/std:c++20 /permissive-)
else()
    add_compile_options(-std=c++20)
endif()

# Windows and Asio definitions
add_compile_definitions(
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

# Find Crow, libpqxx, and OpenSSL from local vcpkg
find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Clean Architecture Multi-Module Subprojects
add_subdirectory ("Domain")
add_subdirectory ("Application")
add_subdirectory ("Infrastructure")
add_subdirectory ("Presentation")
add_subdirectory ("Core")
add_subdirectory ("Tests")

