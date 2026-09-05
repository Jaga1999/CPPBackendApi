#pragma once

#include <chrono>
#include <crow.h>
#include <iostream>
#include <string>

namespace Presentation::Middleware {

enum class AppLogLevel {
    Verbose = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5
};

inline AppLogLevel parseLogLevel(std::string_view levelStr) {
    if (levelStr == "verbose" || levelStr == "VERBOSE" || levelStr == "trace" || levelStr == "TRACE") return AppLogLevel::Verbose;
    if (levelStr == "debug" || levelStr == "DEBUG") return AppLogLevel::Debug;
    if (levelStr == "info" || levelStr == "INFO") return AppLogLevel::Info;
    if (levelStr == "warning" || levelStr == "warn" || levelStr == "WARN" || levelStr == "WARNING") return AppLogLevel::Warning;
    if (levelStr == "error" || levelStr == "ERROR" || levelStr == "err" || levelStr == "ERR") return AppLogLevel::Error;
    if (levelStr == "critical" || levelStr == "CRITICAL" || levelStr == "fatal" || levelStr == "FATAL") return AppLogLevel::Critical;
    return AppLogLevel::Info;
}

inline std::string logLevelToString(AppLogLevel level) {
    switch (level) {
        case AppLogLevel::Verbose: return "VERBOSE";
        case AppLogLevel::Debug: return "DEBUG";
        case AppLogLevel::Info: return "INFO";
        case AppLogLevel::Warning: return "WARNING";
        case AppLogLevel::Error: return "ERROR";
        case AppLogLevel::Critical: return "CRITICAL";
    }
    return "INFO";
}

inline crow::LogLevel toCrowLogLevel(AppLogLevel level) {
    switch (level) {
        case AppLogLevel::Verbose:
        case AppLogLevel::Debug: return crow::LogLevel::Debug;
        case AppLogLevel::Info: return crow::LogLevel::Info;
        case AppLogLevel::Warning: return crow::LogLevel::Warning;
        case AppLogLevel::Error: return crow::LogLevel::Error;
        case AppLogLevel::Critical: return crow::LogLevel::Critical;
    }
    return crow::LogLevel::Info;
}

struct LoggingMiddleware {
    struct context {
        std::chrono::steady_clock::time_point startTime;
    };

    static inline AppLogLevel s_currentLevel{AppLogLevel::Info};

    static void setLogLevel(AppLogLevel level) {
        s_currentLevel = level;
    }

    static AppLogLevel getLogLevel() noexcept {
        return s_currentLevel;
    }

    void before_handle(crow::request& req, crow::response& /*res*/, context& ctx) {
        ctx.startTime = std::chrono::steady_clock::now();
        if (s_currentLevel <= AppLogLevel::Verbose) {
            CROW_LOG_DEBUG << "[VERBOSE] Incoming " << crow::method_name(req.method)
                           << " " << req.url << " (content-length: " << req.body.size() << ")";
        }
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(now - ctx.startTime).count();
        const double elapsedMs = static_cast<double>(elapsedUs) / 1000.0;

        bool shouldLog = false;
        if (s_currentLevel <= AppLogLevel::Info) {
            shouldLog = true;
        } else if (s_currentLevel == AppLogLevel::Warning) {
            shouldLog = (res.code >= 400);
        } else if (s_currentLevel >= AppLogLevel::Error) {
            shouldLog = (res.code >= 500);
        }

        if (!shouldLog) return;

        if (res.code >= 500) {
            CROW_LOG_ERROR << "[" << crow::method_name(req.method) << "] "
                           << req.url << " -> Status " << res.code
                           << " (" << elapsedMs << " ms) [SERVER ERROR]";
        } else if (res.code >= 400) {
            CROW_LOG_WARNING << "[" << crow::method_name(req.method) << "] "
                             << req.url << " -> Status " << res.code
                             << " (" << elapsedMs << " ms) [CLIENT ERROR]";
        } else {
            if (s_currentLevel == AppLogLevel::Verbose) {
                CROW_LOG_INFO << "[VERBOSE] [" << crow::method_name(req.method) << "] "
                              << req.url << " -> Status " << res.code
                              << " (" << elapsedMs << " ms) [bytes: " << res.body.size() << "]";
            } else {
                CROW_LOG_INFO << "[" << crow::method_name(req.method) << "] "
                              << req.url << " -> Status " << res.code
                              << " (" << elapsedMs << " ms)";
            }
        }
    }
};

} // namespace Presentation::Middleware
