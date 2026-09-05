#include "Presentation/Common/HttpResponseHelper.h"

namespace Presentation::Common {

crow::json::wvalue HttpResponseHelper::serializeTodoDto(const Application::DTOs::TodoResponse& dto) {
    crow::json::wvalue val;
    val["id"] = dto.id;
    val["title"] = dto.title;
    val["description"] = dto.description;
    val["completed"] = dto.completed;
    val["createdAt"] = dto.createdAt;
    val["updatedAt"] = dto.updatedAt;
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeHealthDto(const Application::DTOs::HealthResponse& dto) {
    crow::json::wvalue val;
    val["status"] = dto.status;
    val["service"] = dto.service;
    val["version"] = dto.version;
    val["uptimeSeconds"] = dto.uptimeSeconds;
    val["timestamp"] = dto.timestamp;
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeCacheDto(const Application::DTOs::CacheResponse& dto) {
    crow::json::wvalue val;
    val["key"] = dto.key;
    val["value"] = dto.value;
    val["ttlSeconds"] = dto.ttlSeconds;
    val["createdAt"] = dto.createdAt;
    val["expiresAt"] = dto.expiresAt;
    val["isExpired"] = dto.isExpired;
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeQueueMessageDto(const Application::DTOs::QueueMessageResponse& dto) {
    crow::json::wvalue val;
    val["id"] = dto.id;
    val["topic"] = dto.topic;
    auto jsonPayload = crow::json::load(dto.payload);
    if (jsonPayload) {
        val["payload"] = jsonPayload;
    } else {
        val["payload"] = dto.payload;
    }
    val["status"] = dto.status;
    val["retryCount"] = dto.retryCount;
    val["createdAt"] = dto.createdAt;
    if (dto.processedAt.has_value()) {
        val["processedAt"] = *dto.processedAt;
    } else {
        val["processedAt"] = nullptr;
    }
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeQueueMetricsDto(const Application::DTOs::QueueMetricsResponse& dto) {
    crow::json::wvalue val;
    val["topic"] = dto.topic;
    val["pending"] = dto.pending;
    val["processing"] = dto.processing;
    val["completed"] = dto.completed;
    val["failed"] = dto.failed;
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeDocumentDto(const Application::DTOs::DocumentResponse& dto) {
    crow::json::wvalue val;
    val["id"] = dto.id;
    val["collection"] = dto.collection;
    auto jsonData = crow::json::load(dto.data);
    if (jsonData) {
        val["data"] = jsonData;
    } else {
        val["data"] = dto.data;
    }
    val["createdAt"] = dto.createdAt;
    val["updatedAt"] = dto.updatedAt;
    return val;
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::TodoResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeTodoDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<std::vector<Application::DTOs::TodoResponse>>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    crow::json::wvalue::list list;
    if (resp.data.has_value()) {
        list.reserve(resp.data->size());
        for (const auto& item : *resp.data) {
            list.push_back(serializeTodoDto(item));
        }
    }
    envelope["data"] = std::move(list);
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::HealthResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeHealthDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<void>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    envelope["data"] = nullptr;
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::CacheResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeCacheDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<size_t>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = static_cast<uint64_t>(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::QueueMessageResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeQueueMessageDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<std::vector<Application::DTOs::QueueMetricsResponse>>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    crow::json::wvalue::list list;
    if (resp.data.has_value()) {
        list.reserve(resp.data->size());
        for (const auto& item : *resp.data) {
            list.push_back(serializeQueueMetricsDto(item));
        }
    }
    envelope["data"] = std::move(list);
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::DocumentResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeDocumentDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<std::vector<Application::DTOs::DocumentResponse>>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    crow::json::wvalue::list list;
    if (resp.data.has_value()) {
        list.reserve(resp.data->size());
        for (const auto& item : *resp.data) {
            list.push_back(serializeDocumentDto(item));
        }
    }
    envelope["data"] = std::move(list);
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::error(int statusCode, std::string message, std::vector<std::string> errorDetails) {
    crow::json::wvalue envelope;
    envelope["success"] = false;
    envelope["statusCode"] = statusCode;
    envelope["message"] = std::move(message);
    envelope["timestamp"] = Application::Common::currentTimestampIso8601();
    envelope["data"] = nullptr;

    crow::json::wvalue::list errorsList;
    errorsList.reserve(errorDetails.size());
    for (auto& err : errorDetails) {
        errorsList.push_back(std::move(err));
    }
    envelope["errors"] = std::move(errorsList);

    return crow::response(statusCode, envelope);
}

crow::response HttpResponseHelper::error(const Domain::Common::DomainError& domainError) {
    std::vector<std::string> details = domainError.details;
    if (details.empty()) {
        details.push_back(domainError.message);
    }
    return error(domainError.statusCode, domainError.message, std::move(details));
}

Domain::Common::Result<Application::DTOs::CreateTodoRequest> HttpResponseHelper::parseCreateTodoRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::CreateTodoRequest>::err(
            Domain::Common::DomainError{.message = "Malformed or missing request body", .statusCode = 400, .details = {"Request body cannot be empty."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::CreateTodoRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON syntax", .statusCode = 400, .details = {"Failed to parse JSON body."}}
        );
    }
    std::vector<std::string> errors;
    if (!json.has("title")) {
        errors.push_back("Field 'title' is required.");
    } else if (json["title"].t() != crow::json::type::String) {
        errors.push_back("Field 'title' must be a string.");
    }
    if (json.has("description") && json["description"].t() != crow::json::type::String) {
        errors.push_back("Field 'description' must be a string.");
    }
    if (!errors.empty()) {
        return Domain::Common::Result<Application::DTOs::CreateTodoRequest>::err(
            Domain::Common::DomainError{.message = "JSON payload validation failed", .statusCode = 400, .details = std::move(errors)}
        );
    }
    Application::DTOs::CreateTodoRequest r;
    r.title = std::string(json["title"].s());
    if (json.has("description")) {
        r.description = std::string(json["description"].s());
    }
    return Domain::Common::Result<Application::DTOs::CreateTodoRequest>::ok(std::move(r));
}

Domain::Common::Result<Application::DTOs::UpdateTodoRequest> HttpResponseHelper::parseUpdateTodoRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::UpdateTodoRequest>::err(
            Domain::Common::DomainError{.message = "Malformed or missing request body", .statusCode = 400, .details = {"Request body cannot be empty."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::UpdateTodoRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON syntax", .statusCode = 400, .details = {"Failed to parse JSON body."}}
        );
    }
    std::vector<std::string> errors;
    Application::DTOs::UpdateTodoRequest r;
    if (json.has("title")) {
        if (json["title"].t() != crow::json::type::String) {
            errors.push_back("Field 'title' must be a string.");
        } else {
            r.title = std::string(json["title"].s());
        }
    }
    if (json.has("description")) {
        if (json["description"].t() != crow::json::type::String) {
            errors.push_back("Field 'description' must be a string.");
        } else {
            r.description = std::string(json["description"].s());
        }
    }
    if (json.has("completed")) {
        if (json["completed"].t() != crow::json::type::True && json["completed"].t() != crow::json::type::False) {
            errors.push_back("Field 'completed' must be a boolean.");
        } else {
            r.completed = json["completed"].b();
        }
    }
    if (!errors.empty()) {
        return Domain::Common::Result<Application::DTOs::UpdateTodoRequest>::err(
            Domain::Common::DomainError{.message = "JSON payload validation failed", .statusCode = 400, .details = std::move(errors)}
        );
    }
    return Domain::Common::Result<Application::DTOs::UpdateTodoRequest>::ok(std::move(r));
}

Domain::Common::Result<Application::DTOs::SetCacheRequest> HttpResponseHelper::parseSetCacheRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::SetCacheRequest>::err(
            Domain::Common::DomainError{.message = "Empty request body", .statusCode = 400, .details = {"JSON body required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::SetCacheRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON syntax", .statusCode = 400, .details = {"Failed to parse JSON."}}
        );
    }
    std::vector<std::string> errors;
    if (!json.has("key") || json["key"].t() != crow::json::type::String) {
        errors.push_back("Field 'key' is required and must be a string.");
    }
    if (!json.has("value") || json["value"].t() != crow::json::type::String) {
        errors.push_back("Field 'value' is required and must be a string.");
    }
    if (!errors.empty()) {
        return Domain::Common::Result<Application::DTOs::SetCacheRequest>::err(
            Domain::Common::DomainError{.message = "Validation failed for cache set", .statusCode = 400, .details = std::move(errors)}
        );
    }
    Application::DTOs::SetCacheRequest r;
    r.key = std::string(json["key"].s());
    r.value = std::string(json["value"].s());
    if (json.has("ttlSeconds") && json["ttlSeconds"].t() == crow::json::type::Number) {
        r.ttlSeconds = static_cast<int>(json["ttlSeconds"].i());
    }
    return Domain::Common::Result<Application::DTOs::SetCacheRequest>::ok(std::move(r));
}

Domain::Common::Result<Application::DTOs::PublishMessageRequest> HttpResponseHelper::parsePublishMessageRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::PublishMessageRequest>::err(
            Domain::Common::DomainError{.message = "Empty request body", .statusCode = 400, .details = {"JSON body required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::PublishMessageRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON syntax", .statusCode = 400, .details = {"Failed to parse JSON."}}
        );
    }
    std::vector<std::string> errors;
    if (!json.has("topic") || json["topic"].t() != crow::json::type::String) {
        errors.push_back("Field 'topic' is required and must be a string.");
    }
    if (!json.has("payload")) {
        errors.push_back("Field 'payload' is required.");
    }
    if (!errors.empty()) {
        return Domain::Common::Result<Application::DTOs::PublishMessageRequest>::err(
            Domain::Common::DomainError{.message = "Validation failed for queue publish", .statusCode = 400, .details = std::move(errors)}
        );
    }
    Application::DTOs::PublishMessageRequest r;
    r.topic = std::string(json["topic"].s());
    if (json["payload"].t() == crow::json::type::String) {
        r.payload = std::string(json["payload"].s());
    } else {
        crow::json::wvalue w(json["payload"]);
        r.payload = w.dump();
    }
    return Domain::Common::Result<Application::DTOs::PublishMessageRequest>::ok(std::move(r));
}

Domain::Common::Result<Application::DTOs::PollMessageRequest> HttpResponseHelper::parsePollMessageRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::PollMessageRequest>::err(
            Domain::Common::DomainError{.message = "Empty request body", .statusCode = 400, .details = {"JSON body required with field 'topic'."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json || !json.has("topic") || json["topic"].t() != crow::json::type::String) {
        return Domain::Common::Result<Application::DTOs::PollMessageRequest>::err(
            Domain::Common::DomainError{.message = "Validation failed", .statusCode = 400, .details = {"Field 'topic' is required and must be a string."}}
        );
    }
    return Domain::Common::Result<Application::DTOs::PollMessageRequest>::ok(
        Application::DTOs::PollMessageRequest{.topic = std::string(json["topic"].s())}
    );
}

Domain::Common::Result<Application::DTOs::CreateDocumentRequest> HttpResponseHelper::parseCreateDocumentRequest(std::string_view collection, const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::CreateDocumentRequest>::err(
            Domain::Common::DomainError{.message = "Empty body", .statusCode = 400, .details = {"JSON document required in body."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::CreateDocumentRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON syntax", .statusCode = 400, .details = {"Document must be valid JSON."}}
        );
    }
    return Domain::Common::Result<Application::DTOs::CreateDocumentRequest>::ok(
        Application::DTOs::CreateDocumentRequest{
            .collection = std::string(collection),
            .data = req.body
        }
    );
}

Domain::Common::Result<Application::DTOs::QueryDocumentRequest> HttpResponseHelper::parseQueryDocumentRequest(std::string_view collection, const crow::request& req) {
    std::string filter = req.body.empty() ? "{}" : req.body;
    auto json = crow::json::load(filter);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::QueryDocumentRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON query filter", .statusCode = 400, .details = {"Query filter must be valid JSON."}}
        );
    }
    return Domain::Common::Result<Application::DTOs::QueryDocumentRequest>::ok(
        Application::DTOs::QueryDocumentRequest{
            .collection = std::string(collection),
            .filterJson = std::move(filter)
        }
    );
}

crow::json::wvalue HttpResponseHelper::serializeRegisterDto(const Application::DTOs::RegisterResponse& dto) {
    crow::json::wvalue val;
    val["id"] = dto.id;
    val["email"] = dto.email;
    val["role"] = dto.role;
    val["createdAt"] = dto.createdAt;
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeTokenDto(const Application::DTOs::TokenResponse& dto) {
    crow::json::wvalue val;
    val["accessToken"] = dto.accessToken;
    val["refreshToken"] = dto.refreshToken;
    val["tokenType"] = dto.tokenType;
    val["expiresIn"] = dto.expiresIn;
    val["sessionId"] = dto.sessionId;

    crow::json::wvalue userObj;
    userObj["id"] = dto.user.id;
    userObj["email"] = dto.user.email;
    userObj["role"] = dto.user.role;
    userObj["authProvider"] = dto.user.authProvider;
    userObj["avatarUrl"] = dto.user.avatarUrl;
    userObj["googleLinked"] = dto.user.googleLinked;
    val["user"] = std::move(userObj);

    return val;
}

crow::json::wvalue HttpResponseHelper::serializeGoogleAuthUrlDto(const Application::DTOs::GoogleAuthUrlResponse& dto) {
    crow::json::wvalue val;
    val["authUrl"] = dto.authUrl;
    val["state"] = dto.state;
    val["codeVerifier"] = dto.codeVerifier;
    val["codeChallenge"] = dto.codeChallenge;
    return val;
}

crow::json::wvalue HttpResponseHelper::serializeSessionDto(const Application::DTOs::SessionResponse& dto) {
    crow::json::wvalue val;
    val["id"] = dto.id;
    val["userId"] = dto.userId;
    val["device"] = dto.device;
    val["ipAddress"] = dto.ipAddress;
    val["jti"] = dto.jti;
    val["createdAt"] = dto.createdAt;
    val["lastSeenAt"] = dto.lastSeenAt;
    val["expiresAt"] = dto.expiresAt;
    val["current"] = dto.current;
    val["status"] = dto.status;
    return val;
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::RegisterResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeRegisterDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::TokenResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeTokenDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::SessionResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeSessionDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<std::vector<Application::DTOs::SessionResponse>>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;

    std::vector<crow::json::wvalue> list;
    if (resp.data.has_value()) {
        list.reserve(resp.data->size());
        for (const auto& item : *resp.data) {
            list.push_back(serializeSessionDto(item));
        }
    }
    envelope["data"] = std::move(list);
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

crow::response HttpResponseHelper::success(const Application::Common::ApiResponse<Application::DTOs::GoogleAuthUrlResponse>& resp) {
    crow::json::wvalue envelope;
    envelope["success"] = resp.success;
    envelope["statusCode"] = resp.statusCode;
    envelope["message"] = resp.message;
    envelope["timestamp"] = resp.timestamp;
    if (resp.data.has_value()) {
        envelope["data"] = serializeGoogleAuthUrlDto(*resp.data);
    } else {
        envelope["data"] = nullptr;
    }
    envelope["errors"] = crow::json::wvalue::list{};
    return crow::response(resp.statusCode, envelope);
}

Domain::Common::Result<Application::DTOs::RegisterRequest> HttpResponseHelper::parseRegisterRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::RegisterRequest>::err(
            Domain::Common::DomainError{.message = "Empty body", .statusCode = 400, .details = {"JSON registration payload required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::RegisterRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON", .statusCode = 400, .details = {"Payload must be valid JSON."}}
        );
    }

    std::string email = json.has("email") && json["email"].t() == crow::json::type::String ? std::string(json["email"].s()) : "";
    std::string password = json.has("password") && json["password"].t() == crow::json::type::String ? std::string(json["password"].s()) : "";
    std::string role = json.has("role") && json["role"].t() == crow::json::type::String ? std::string(json["role"].s()) : "user";

    return Domain::Common::Result<Application::DTOs::RegisterRequest>::ok(
        Application::DTOs::RegisterRequest{
            .email = std::move(email),
            .password = std::move(password),
            .role = std::move(role)
        }
    );
}

Domain::Common::Result<Application::DTOs::LoginRequest> HttpResponseHelper::parseLoginRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::LoginRequest>::err(
            Domain::Common::DomainError{.message = "Empty body", .statusCode = 400, .details = {"JSON login payload required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::LoginRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON", .statusCode = 400, .details = {"Payload must be valid JSON."}}
        );
    }

    std::string email = json.has("email") && json["email"].t() == crow::json::type::String ? std::string(json["email"].s()) : "";
    std::string password = json.has("password") && json["password"].t() == crow::json::type::String ? std::string(json["password"].s()) : "";

    return Domain::Common::Result<Application::DTOs::LoginRequest>::ok(
        Application::DTOs::LoginRequest{
            .email = std::move(email),
            .password = std::move(password)
        }
    );
}

Domain::Common::Result<Application::DTOs::GoogleLoginRequest> HttpResponseHelper::parseGoogleLoginRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::GoogleLoginRequest>::err(
            Domain::Common::DomainError{.message = "Empty body", .statusCode = 400, .details = {"JSON login payload required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::GoogleLoginRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON", .statusCode = 400, .details = {"Payload must be valid JSON."}}
        );
    }

    std::string idToken = json.has("idToken") && json["idToken"].t() == crow::json::type::String ? std::string(json["idToken"].s()) : "";
    std::string code = json.has("code") && json["code"].t() == crow::json::type::String ? std::string(json["code"].s()) : "";
    std::string codeVerifier = json.has("codeVerifier") && json["codeVerifier"].t() == crow::json::type::String ? std::string(json["codeVerifier"].s()) : "";
    std::string state = json.has("state") && json["state"].t() == crow::json::type::String ? std::string(json["state"].s()) : "";
    std::string deviceName = json.has("deviceName") && json["deviceName"].t() == crow::json::type::String ? std::string(json["deviceName"].s()) : "Unknown Device";
    std::string clientType = json.has("clientType") && json["clientType"].t() == crow::json::type::String ? std::string(json["clientType"].s()) : "browser";

    if (idToken.empty() && code.empty()) {
        return Domain::Common::Result<Application::DTOs::GoogleLoginRequest>::err(
            Domain::Common::DomainError{.message = "Validation failed", .statusCode = 400, .details = {"Either 'idToken' or 'code' must be provided."}}
        );
    }

    return Domain::Common::Result<Application::DTOs::GoogleLoginRequest>::ok(
        Application::DTOs::GoogleLoginRequest{
            .idToken = std::move(idToken),
            .code = std::move(code),
            .codeVerifier = std::move(codeVerifier),
            .state = std::move(state),
            .deviceName = std::move(deviceName),
            .clientType = std::move(clientType)
        }
    );
}

Domain::Common::Result<Application::DTOs::SetPasswordRequest> HttpResponseHelper::parseSetPasswordRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::SetPasswordRequest>::err(
            Domain::Common::DomainError{.message = "Empty body", .statusCode = 400, .details = {"JSON payload with 'password' required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::SetPasswordRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON", .statusCode = 400, .details = {"Payload must be valid JSON."}}
        );
    }

    std::string password = json.has("password") && json["password"].t() == crow::json::type::String ? std::string(json["password"].s()) : "";
    if (password.empty()) {
        return Domain::Common::Result<Application::DTOs::SetPasswordRequest>::err(
            Domain::Common::DomainError{.message = "Validation failed", .statusCode = 400, .details = {"Field 'password' is required."}}
        );
    }

    return Domain::Common::Result<Application::DTOs::SetPasswordRequest>::ok(
        Application::DTOs::SetPasswordRequest{.password = std::move(password)}
    );
}

Domain::Common::Result<Application::DTOs::RefreshTokenRequest> HttpResponseHelper::parseRefreshTokenRequest(const crow::request& req) {
    if (req.body.empty()) {
        return Domain::Common::Result<Application::DTOs::RefreshTokenRequest>::err(
            Domain::Common::DomainError{.message = "Empty body", .statusCode = 400, .details = {"JSON payload with 'refreshToken' required."}}
        );
    }
    auto json = crow::json::load(req.body);
    if (!json) {
        return Domain::Common::Result<Application::DTOs::RefreshTokenRequest>::err(
            Domain::Common::DomainError{.message = "Invalid JSON", .statusCode = 400, .details = {"Payload must be valid JSON."}}
        );
    }

    std::string token = json.has("refreshToken") && json["refreshToken"].t() == crow::json::type::String ? std::string(json["refreshToken"].s()) : "";

    return Domain::Common::Result<Application::DTOs::RefreshTokenRequest>::ok(
        Application::DTOs::RefreshTokenRequest{.refreshToken = std::move(token)}
    );
}

Domain::Common::Result<Application::DTOs::AdminRevokeUserSessionsRequest> HttpResponseHelper::parseAdminRevokeUserSessionsRequest(const crow::request& req) {
    std::string reason = "Administrative revocation";
    if (!req.body.empty()) {
        auto json = crow::json::load(req.body);
        if (json && json.has("reason") && json["reason"].t() == crow::json::type::String) {
            reason = std::string(json["reason"].s());
        }
    }
    return Domain::Common::Result<Application::DTOs::AdminRevokeUserSessionsRequest>::ok(
        Application::DTOs::AdminRevokeUserSessionsRequest{.reason = std::move(reason)}
    );
}

Domain::Common::Result<Middleware::AuthenticatedUser> HttpResponseHelper::extractAuthenticatedUser(
    const crow::request& req,
    const Application::Security::IJwtService& jwtService,
    const Domain::Repositories::ISessionRepository& sessionRepo,
    const Domain::Repositories::IUserRepository& userRepo
) {
    std::string authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.rfind("Bearer ", 0) != 0) {
        return Domain::Common::Result<Middleware::AuthenticatedUser>::err(
            Domain::Common::DomainError{.message = "Unauthorized", .statusCode = 401, .details = {"Missing or invalid 'Authorization: Bearer <token>' header."}}
        );
    }

    std::string token = authHeader.substr(7);
    auto claimsRes = jwtService.validateAccessToken(token);
    if (claimsRes.isErr()) {
        return Domain::Common::Result<Middleware::AuthenticatedUser>::err(claimsRes.error());
    }

    const auto& claims = claimsRes.value();

    // Check if token's JTI was specifically revoked
    if (sessionRepo.isTokenRevoked(claims.jti)) {
        return Domain::Common::Result<Middleware::AuthenticatedUser>::err(
            Domain::Common::DomainError{.message = "Token revoked", .statusCode = 401, .details = {"Access token has been revoked."}}
        );
    }

    // Check if parent session is still active
    auto sessionOpt = sessionRepo.findById(claims.sid);
    if (!sessionOpt.has_value() || sessionOpt->isRevoked() || sessionOpt->isExpired()) {
        return Domain::Common::Result<Middleware::AuthenticatedUser>::err(
            Domain::Common::DomainError{.message = "Session terminated", .statusCode = 401, .details = {"Session has been revoked or has expired."}}
        );
    }

    // Check user account status
    auto userOpt = userRepo.findById(claims.sub);
    if (!userOpt.has_value() || !userOpt->isActive) {
        return Domain::Common::Result<Middleware::AuthenticatedUser>::err(
            Domain::Common::DomainError{.message = "Account disabled", .statusCode = 401, .details = {"User account is inactive or disabled."}}
        );
    }

    Middleware::AuthenticatedUser authUser{
        .id = userOpt->id,
        .email = userOpt->email,
        .role = userOpt->role,
        .sessionId = claims.sid,
        .jti = claims.jti
    };

    return Domain::Common::Result<Middleware::AuthenticatedUser>::ok(std::move(authUser));
}

} // namespace Presentation::Common
