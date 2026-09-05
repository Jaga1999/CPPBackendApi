#pragma once

#include "Domain/Entities/DocumentEntity.h"
#include <chrono>
#include <format>
#include <string>

namespace Application::DTOs {

struct CreateDocumentRequest {
    std::string collection;
    std::string data; // JSON string
};

struct UpdateDocumentRequest {
    std::string data; // Replacement or patch JSON
};

struct QueryDocumentRequest {
    std::string collection;
    std::string filterJson; // JSONB containment filter e.g. {"status":"active"}
};

struct DocumentResponse {
    std::string id;
    std::string collection;
    std::string data;
    std::string createdAt;
    std::string updatedAt;

    static DocumentResponse fromDomain(const Domain::Entities::DocumentEntity& doc) {
        auto formatTimestamp = [](const std::chrono::system_clock::time_point& tp) -> std::string {
            try {
                return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(tp));
            } catch (...) {
                return "1970-01-01T00:00:00Z";
            }
        };

        return DocumentResponse{
            .id = doc.id,
            .collection = doc.collection,
            .data = doc.data,
            .createdAt = formatTimestamp(doc.createdAt),
            .updatedAt = formatTimestamp(doc.updatedAt)
        };
    }
};

} // namespace Application::DTOs
