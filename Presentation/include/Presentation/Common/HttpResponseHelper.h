#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/AuthDtos.h"
#include "Application/DTOs/GoogleAuthDtos.h"
#include "Application/DTOs/CacheDtos.h"
#include "Application/DTOs/DocumentDtos.h"
#include "Application/DTOs/HealthDto.h"
#include "Application/DTOs/QueueDtos.h"
#include "Application/DTOs/TodoDtos.h"
#include "Application/Security/IJwtService.h"
#include "Domain/Common/Result.h"
#include "Domain/Repositories/ISessionRepository.h"
#include "Domain/Repositories/IUserRepository.h"
#include "Presentation/Middleware/AuthContext.h"
#include <crow.h>
#include <string>
#include <vector>

namespace Presentation::Common {

class HttpResponseHelper {
public:
    // DTO Serializers
    static crow::json::wvalue serializeTodoDto(const Application::DTOs::TodoResponse& dto);
    static crow::json::wvalue serializeHealthDto(const Application::DTOs::HealthResponse& dto);
    static crow::json::wvalue serializeCacheDto(const Application::DTOs::CacheResponse& dto);
    static crow::json::wvalue serializeQueueMessageDto(const Application::DTOs::QueueMessageResponse& dto);
    static crow::json::wvalue serializeQueueMetricsDto(const Application::DTOs::QueueMetricsResponse& dto);
    static crow::json::wvalue serializeDocumentDto(const Application::DTOs::DocumentResponse& dto);
    static crow::json::wvalue serializeRegisterDto(const Application::DTOs::RegisterResponse& dto);
    static crow::json::wvalue serializeTokenDto(const Application::DTOs::TokenResponse& dto);
    static crow::json::wvalue serializeSessionDto(const Application::DTOs::SessionResponse& dto);
    static crow::json::wvalue serializeGoogleAuthUrlDto(const Application::DTOs::GoogleAuthUrlResponse& dto);

    // Standard Success Envelopes
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::TodoResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<std::vector<Application::DTOs::TodoResponse>>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::HealthResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<void>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::CacheResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<size_t>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::QueueMessageResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<std::vector<Application::DTOs::QueueMetricsResponse>>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::DocumentResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<std::vector<Application::DTOs::DocumentResponse>>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::RegisterResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::TokenResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::SessionResponse>& resp);
    static crow::response success(const Application::Common::ApiResponse<std::vector<Application::DTOs::SessionResponse>>& resp);
    static crow::response success(const Application::Common::ApiResponse<Application::DTOs::GoogleAuthUrlResponse>& resp);

    // Standard Error Envelopes
    static crow::response error(
        int statusCode,
        std::string message,
        std::vector<std::string> errorDetails = {}
    );

    static crow::response error(const Domain::Common::DomainError& domainError);

    // Request Parsing helpers
    static Domain::Common::Result<Application::DTOs::CreateTodoRequest> parseCreateTodoRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::UpdateTodoRequest> parseUpdateTodoRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::SetCacheRequest> parseSetCacheRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::PublishMessageRequest> parsePublishMessageRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::PollMessageRequest> parsePollMessageRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::CreateDocumentRequest> parseCreateDocumentRequest(std::string_view collection, const crow::request& req);
    static Domain::Common::Result<Application::DTOs::QueryDocumentRequest> parseQueryDocumentRequest(std::string_view collection, const crow::request& req);

    static Domain::Common::Result<Application::DTOs::RegisterRequest> parseRegisterRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::LoginRequest> parseLoginRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::GoogleLoginRequest> parseGoogleLoginRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::SetPasswordRequest> parseSetPasswordRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::RefreshTokenRequest> parseRefreshTokenRequest(const crow::request& req);
    static Domain::Common::Result<Application::DTOs::AdminRevokeUserSessionsRequest> parseAdminRevokeUserSessionsRequest(const crow::request& req);

    // Authentication Context Extractor
    static Domain::Common::Result<Middleware::AuthenticatedUser> extractAuthenticatedUser(
        const crow::request& req,
        const Application::Security::IJwtService& jwtService,
        const Domain::Repositories::ISessionRepository& sessionRepo,
        const Domain::Repositories::IUserRepository& userRepo
    );
};

} // namespace Presentation::Common
