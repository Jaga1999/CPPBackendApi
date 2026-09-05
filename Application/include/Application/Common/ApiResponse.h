#pragma once

#include <chrono>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace Application::Common {

inline std::string currentTimestampIso8601() {
    try {
        const auto now = std::chrono::system_clock::now();
        return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(now));
    } catch (...) {
        return "1970-01-01T00:00:00Z";
    }
}

/**
 * @brief Standardized API Response object returned to all API consumers.
 * Unifies both success payloads and error payloads across the entire application.
 */
template <typename T>
struct ApiResponse {
    bool success{true};
    int statusCode{200};
    std::string message{"Success"};
    std::optional<T> data{std::nullopt};
    std::vector<std::string> errors{};
    std::string timestamp{currentTimestampIso8601()};

    static ApiResponse<T> ok(T dataVal, std::string msg = "Success", int code = 200) {
        return ApiResponse<T>{
            .success = true,
            .statusCode = code,
            .message = std::move(msg),
            .data = std::move(dataVal),
            .errors = {},
            .timestamp = currentTimestampIso8601()
        };
    }

    static ApiResponse<T> created(T dataVal, std::string msg = "Resource created successfully") {
        return ApiResponse<T>{
            .success = true,
            .statusCode = 201,
            .message = std::move(msg),
            .data = std::move(dataVal),
            .errors = {},
            .timestamp = currentTimestampIso8601()
        };
    }

    static ApiResponse<T> fail(std::string msg, std::vector<std::string> errList = {}, int code = 400) {
        return ApiResponse<T>{
            .success = false,
            .statusCode = code,
            .message = std::move(msg),
            .data = std::nullopt,
            .errors = std::move(errList),
            .timestamp = currentTimestampIso8601()
        };
    }
};

/**
 * @brief Specialization of ApiResponse for void / empty data payloads.
 */
template <>
struct ApiResponse<void> {
    bool success{true};
    int statusCode{200};
    std::string message{"Success"};
    std::vector<std::string> errors{};
    std::string timestamp{currentTimestampIso8601()};

    static ApiResponse<void> ok(std::string msg = "Success", int code = 200) {
        return ApiResponse<void>{
            .success = true,
            .statusCode = code,
            .message = std::move(msg),
            .errors = {},
            .timestamp = currentTimestampIso8601()
        };
    }

    static ApiResponse<void> created(std::string msg = "Resource created successfully") {
        return ApiResponse<void>{
            .success = true,
            .statusCode = 201,
            .message = std::move(msg),
            .errors = {},
            .timestamp = currentTimestampIso8601()
        };
    }

    static ApiResponse<void> fail(std::string msg, std::vector<std::string> errList = {}, int code = 400) {
        return ApiResponse<void>{
            .success = false,
            .statusCode = code,
            .message = std::move(msg),
            .errors = std::move(errList),
            .timestamp = currentTimestampIso8601()
        };
    }
};

} // namespace Application::Common
