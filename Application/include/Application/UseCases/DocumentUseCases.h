#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/DocumentDtos.h"
#include "Domain/Common/Result.h"
#include "Domain/Repositories/IDocumentRepository.h"
#include <memory>
#include <string>
#include <vector>

namespace Application::UseCases {

class DocumentUseCases {
public:
    explicit DocumentUseCases(std::shared_ptr<Domain::Repositories::IDocumentRepository> repository);

    Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>> create(
        DTOs::CreateDocumentRequest request
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>> getById(
        const std::string& collection,
        const std::string& id
    );

    Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::DocumentResponse>>> query(
        DTOs::QueryDocumentRequest request
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>> update(
        const std::string& collection,
        const std::string& id,
        DTOs::UpdateDocumentRequest request
    );

    Domain::Common::Result<Common::ApiResponse<void>> remove(
        const std::string& collection,
        const std::string& id
    );

private:
    std::shared_ptr<Domain::Repositories::IDocumentRepository> m_repository;
};

} // namespace Application::UseCases
