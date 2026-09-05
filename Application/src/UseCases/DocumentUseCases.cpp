#include "Application/UseCases/DocumentUseCases.h"
#include <format>

namespace Application::UseCases {

DocumentUseCases::DocumentUseCases(std::shared_ptr<Domain::Repositories::IDocumentRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>> DocumentUseCases::create(
    DTOs::CreateDocumentRequest request
) {
    if (request.collection.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for document creation",
                .statusCode = 400,
                .details = {"Field 'collection' cannot be empty."}
            }
        );
    }

    if (request.data.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for document creation",
                .statusCode = 400,
                .details = {"Field 'data' (JSON payload) cannot be empty."}
            }
        );
    }

    auto inserted = m_repository->insert(request.collection, request.data);
    auto response = DTOs::DocumentResponse::fromDomain(inserted);

    return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::ok(
        Common::ApiResponse<DTOs::DocumentResponse>::ok(
            std::move(response),
            std::format("Document stored in collection '{}' with UUID {}", request.collection, inserted.id),
            201
        )
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>> DocumentUseCases::getById(
    const std::string& collection,
    const std::string& id
) {
    if (collection.empty() || id.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Both 'collection' and 'id' must be provided."}
            }
        );
    }

    auto docOpt = m_repository->findById(collection, id);
    if (!docOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::err(
            Domain::Common::DomainError{
                .message = std::format("Document '{}' not found in collection '{}'", id, collection),
                .statusCode = 404,
                .details = {std::format("No document with UUID: {}", id)}
            }
        );
    }

    auto response = DTOs::DocumentResponse::fromDomain(*docOpt);
    return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::ok(
        Common::ApiResponse<DTOs::DocumentResponse>::ok(
            std::move(response),
            "Document retrieved successfully",
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::DocumentResponse>>> DocumentUseCases::query(
    DTOs::QueryDocumentRequest request
) {
    if (request.collection.empty()) {
        return Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::DocumentResponse>>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Field 'collection' is required."}
            }
        );
    }

    if (request.filterJson.empty()) {
        request.filterJson = "{}";
    }

    auto docs = m_repository->queryByFilter(request.collection, request.filterJson);
    std::vector<DTOs::DocumentResponse> responses;
    responses.reserve(docs.size());
    for (const auto& d : docs) {
        responses.push_back(DTOs::DocumentResponse::fromDomain(d));
    }

    return Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::DocumentResponse>>>::ok(
        Common::ApiResponse<std::vector<DTOs::DocumentResponse>>::ok(
            std::move(responses),
            std::format("Found {} documents matching query in '{}'", responses.size(), request.collection),
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>> DocumentUseCases::update(
    const std::string& collection,
    const std::string& id,
    DTOs::UpdateDocumentRequest request
) {
    if (collection.empty() || id.empty() || request.data.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Collection, ID, and updated data payload must be provided."}
            }
        );
    }

    auto updatedOpt = m_repository->update(collection, id, request.data);
    if (!updatedOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::err(
            Domain::Common::DomainError{
                .message = std::format("Document '{}' not found in collection '{}' for update", id, collection),
                .statusCode = 404,
                .details = {std::format("Cannot update document with UUID: {}", id)}
            }
        );
    }

    auto response = DTOs::DocumentResponse::fromDomain(*updatedOpt);
    return Domain::Common::Result<Common::ApiResponse<DTOs::DocumentResponse>>::ok(
        Common::ApiResponse<DTOs::DocumentResponse>::ok(
            std::move(response),
            "Document updated successfully",
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<void>> DocumentUseCases::remove(
    const std::string& collection,
    const std::string& id
) {
    if (collection.empty() || id.empty()) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Both 'collection' and 'id' must be provided."}
            }
        );
    }

    bool removed = m_repository->remove(collection, id);
    if (!removed) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = std::format("Document '{}' not found in collection '{}'", id, collection),
                .statusCode = 404,
                .details = {std::format("Cannot delete document with UUID: {}", id)}
            }
        );
    }

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok(
            std::format("Document '{}' removed from collection '{}'", id, collection),
            200
        )
    );
}

} // namespace Application::UseCases
