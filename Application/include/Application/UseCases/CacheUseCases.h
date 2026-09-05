#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/CacheDtos.h"
#include "Domain/Common/Result.h"
#include "Domain/Repositories/ICacheRepository.h"
#include <cstddef>
#include <memory>
#include <string>

namespace Application::UseCases {

class CacheUseCases {
public:
    explicit CacheUseCases(std::shared_ptr<Domain::Repositories::ICacheRepository> repository);

    Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>> set(DTOs::SetCacheRequest request);
    Domain::Common::Result<Common::ApiResponse<DTOs::CacheResponse>> get(const std::string& key);
    Domain::Common::Result<Common::ApiResponse<void>> remove(const std::string& key);
    Common::ApiResponse<size_t> cleanup();

private:
    std::shared_ptr<Domain::Repositories::ICacheRepository> m_repository;
};

} // namespace Application::UseCases
