#include "Application/UseCases/CacheUseCases.h"
#include <format>

namespace Application::UseCases {

CacheUseCases::CacheUseCases(std::shared_ptr<Domain::Repositories::ICacheRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>> CacheUseCases::set(
    DTOs::SetCacheRequest request
) {
    if (request.key.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for cache set",
                .statusCode = 400,
                .details = {"Cache key cannot be empty."}
            }
        );
    }

    if (request.ttlSeconds <= 0) {
        request.ttlSeconds = 3600; // default 1 hour
    }

    m_repository->set(request.key, request.value, request.ttlSeconds);

    auto entryOpt = m_repository->get(request.key);
    if (!entryOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>>::err(
            Domain::Common::DomainError{
                .message = "Failed to retrieve cache entry after set",
                .statusCode = 500,
                .details = {"Cache store internal write error."}
            }
        );
    }

    auto response = DTOs::CacheResponse::fromDomain(*entryOpt);
    return Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>>::ok(
        Common::ApiResponse<DTOs::CacheResponse>::ok(
            std::move(response),
            std::format("Cache key '{}' set successfully (TTL: {}s)", request.key, request.ttlSeconds),
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>> CacheUseCases::get(const std::string& key) {
    if (key.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Cache key cannot be empty."}
            }
        );
    }

    auto entryOpt = m_repository->get(key);
    if (!entryOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>>::err(
            Domain::Common::DomainError{
                .message = std::format("Cache key '{}' not found or expired", key),
                .statusCode = 404,
                .details = {std::format("No active cache entry for key: {}", key)}
            }
        );
    }

    auto response = DTOs::CacheResponse::fromDomain(*entryOpt);
    return Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>>::ok(
        Common::ApiResponse<DTOs::CacheResponse>::ok(
            std::move(response),
            "Cache hit",
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<void>> CacheUseCases::remove(const std::string& key) {
    if (key.empty()) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Cache key cannot be empty."}
            }
        );
    }

    bool removed = m_repository->remove(key);
    if (!removed) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = std::format("Cache key '{}' not found", key),
                .statusCode = 404,
                .details = {std::format("Cannot delete non-existent cache key: {}", key)}
            }
        );
    }

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok(
            std::format("Cache key '{}' evicted successfully", key),
            200
        )
    );
}

Common::ApiResponse<size_t> CacheUseCases::cleanup() {
    size_t count = m_repository->cleanupExpired();
    return Common::ApiResponse<size_t>::ok(
        count,
        std::format("Expired cache eviction completed: {} items purged", count),
        200
    );
}

} // namespace Application::UseCases
