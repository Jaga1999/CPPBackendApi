# CMakeLists.txt for Core executable (Composition Root)
cmake_minimum_required(VERSION 3.15)
project(Core CXX)

add_executable(Core
    "Core.cpp"
    "Core.h"
)

target_compile_features(Core PRIVATE cxx_std_20)

target_compile_definitions(Core PRIVATE
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_include_directories(Core PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(Core PRIVATE
    Presentation
    Infrastructure
    Application
    Domain
    Crow::Crow
)

if (WIN32)
    target_link_libraries(Core PRIVATE
        ws2_32
        mswsock
    )
endif()

if (MSVC)
    target_link_options(Core PRIVATE "/FORCE:MULTIPLE")
endif()

# Automatically copy all vcpkg runtime DLLs into the executable directory upon build
if (WIN32)
    add_custom_command(TARGET Core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/$<IF:$<CONFIG:Debug>,debug/bin,bin>"
            "$<TARGET_FILE_DIR:Core>"
        COMMENT "Automatically copying all runtime dependency DLLs to executable directory"
    )
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    "Core.cpp"
    "Core.h"
)
