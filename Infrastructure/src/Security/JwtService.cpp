#include "Infrastructure/Security/JwtService.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include <chrono>
#include <crow.h>
#include <sstream>

namespace Infrastructure::Security {

JwtService::JwtService(
    std::shared_ptr<Application::Security::IKeyManager> keyManager,
    std::string issuer,
    std::string audience
)   : m_keyManager(std::move(keyManager)),
      m_issuer(std::move(issuer)),
      m_audience(std::move(audience)) {}

std::string JwtService::createAccessToken(
    std::string_view userId,
    std::string_view role,
    std::string_view sessionId,
    std::string_view jti,
    std::chrono::seconds ttl
) {
    auto signingKey = m_keyManager->getActiveSigningKey();

    const auto now = std::chrono::system_clock::now();
    const auto iat = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto nbf = iat;
    const auto exp = iat + ttl.count();

    // 1. Header
    crow::json::wvalue headerObj;
    headerObj["alg"] = "RS256";
    headerObj["typ"] = "JWT";
    headerObj["kid"] = signingKey.kid;
    std::string headerJson = headerObj.dump();
    std::string headerB64 = OpenSslCrypto::base64UrlEncode(headerJson);

    // 2. Payload Claims
    crow::json::wvalue payloadObj;
    payloadObj["iss"] = m_issuer;
    payloadObj["sub"] = std::string(userId);
    payloadObj["aud"] = m_audience;
    payloadObj["iat"] = iat;
    payloadObj["nbf"] = nbf;
    payloadObj["exp"] = exp;
    payloadObj["jti"] = std::string(jti);
    payloadObj["sid"] = std::string(sessionId);
    payloadObj["role"] = std::string(role);
    std::string payloadJson = payloadObj.dump();
    std::string payloadB64 = OpenSslCrypto::base64UrlEncode(payloadJson);

    // 3. Signature
    std::string signingInput = headerB64 + "." + payloadB64;
    auto sigOpt = OpenSslCrypto::signRs256(signingKey.privateKeyPem, signingInput);
    if (!sigOpt.has_value()) {
        return "";
    }

    return signingInput + "." + *sigOpt;
}

Domain::Common::Result<Application::Security::JwtClaims> JwtService::validateAccessToken(
    std::string_view token
) const {
    // 1. Token structure (must be header.payload.signature)
    size_t firstDot = token.find('.');
    if (firstDot == std::string_view::npos) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Malformed token format", .statusCode = 401, .details = {"Token does not contain expected dot delimiters."}}
        );
    }
    size_t secondDot = token.find('.', firstDot + 1);
    if (secondDot == std::string_view::npos || token.find('.', secondDot + 1) != std::string_view::npos) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Malformed token format", .statusCode = 401, .details = {"Token must have exactly 3 segments."}}
        );
    }

    std::string_view headerB64 = token.substr(0, firstDot);
    std::string_view payloadB64 = token.substr(firstDot + 1, secondDot - (firstDot + 1));
    std::string_view sigB64 = token.substr(secondDot + 1);

    // 2. Decode Header
    auto headerJsonOpt = OpenSslCrypto::base64UrlDecode(headerB64);
    if (!headerJsonOpt.has_value()) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid token header", .statusCode = 401, .details = {"Base64URL decoding failed for header."}}
        );
    }

    auto headerJson = crow::json::load(*headerJsonOpt);
    if (!headerJson) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid token header syntax", .statusCode = 401, .details = {"Header is not valid JSON."}}
        );
    }

    if (!headerJson.has("alg") || headerJson["alg"].s() != "RS256") {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Unsupported token algorithm", .statusCode = 401, .details = {"Only RS256 algorithm is permitted."}}
        );
    }

    if (!headerJson.has("kid") || headerJson["kid"].t() != crow::json::type::String) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Missing key identifier", .statusCode = 401, .details = {"JWS header must contain 'kid'."}}
        );
    }
    std::string kid = std::string(headerJson["kid"].s());

    // 3. Retrieve Verification Key
    auto pubKeyOpt = m_keyManager->getVerificationPublicKeyPem(kid);
    if (!pubKeyOpt.has_value()) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Unknown signing key", .statusCode = 401, .details = {"Key ID '" + kid + "' not recognized."}}
        );
    }

    // 4. Verify Digital Signature
    std::string signingInput = std::string(headerB64) + "." + std::string(payloadB64);
    bool validSig = OpenSslCrypto::verifyRs256(*pubKeyOpt, signingInput, sigB64);
    if (!validSig) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid signature", .statusCode = 401, .details = {"Cryptographic signature verification failed."}}
        );
    }

    // 5. Decode & Validate Payload Claims
    auto payloadJsonOpt = OpenSslCrypto::base64UrlDecode(payloadB64);
    if (!payloadJsonOpt.has_value()) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid token payload", .statusCode = 401, .details = {"Base64URL decoding failed for payload."}}
        );
    }

    auto payloadJson = crow::json::load(*payloadJsonOpt);
    if (!payloadJson) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid token payload syntax", .statusCode = 401, .details = {"Payload is not valid JSON."}}
        );
    }

    // Validate claims presence
    if (!payloadJson.has("iss") || !payloadJson.has("sub") || !payloadJson.has("aud") ||
        !payloadJson.has("exp") || !payloadJson.has("jti") || !payloadJson.has("sid")) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Missing mandatory JWT claims", .statusCode = 401, .details = {"Claims must include iss, sub, aud, exp, jti, sid."}}
        );
    }

    // Validate issuer & audience
    if (payloadJson["iss"].s() != m_issuer) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid token issuer", .statusCode = 401, .details = {"Issuer does not match expected authority."}}
        );
    }
    if (payloadJson["aud"].s() != m_audience) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Invalid token audience", .statusCode = 401, .details = {"Audience does not match expected target."}}
        );
    }

    const auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    int64_t exp = payloadJson["exp"].i();
    if (nowSec >= exp) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Token has expired", .statusCode = 401, .details = {"Token expiration timestamp has passed."}}
        );
    }

    int64_t nbf = payloadJson.has("nbf") ? payloadJson["nbf"].i() : 0;
    if (nowSec < nbf) {
        return Domain::Common::Result<Application::Security::JwtClaims>::err(
            Domain::Common::DomainError{.message = "Token not yet valid", .statusCode = 401, .details = {"Current time is before token nbf timestamp."}}
        );
    }

    Application::Security::JwtClaims claims{
        .iss = std::string(payloadJson["iss"].s()),
        .sub = std::string(payloadJson["sub"].s()),
        .aud = std::string(payloadJson["aud"].s()),
        .iat = payloadJson.has("iat") ? payloadJson["iat"].i() : 0,
        .nbf = nbf,
        .exp = exp,
        .jti = std::string(payloadJson["jti"].s()),
        .sid = std::string(payloadJson["sid"].s()),
        .role = payloadJson.has("role") ? std::string(payloadJson["role"].s()) : "user",
        .kid = std::move(kid)
    };

    return Domain::Common::Result<Application::Security::JwtClaims>::ok(std::move(claims));
}

} // namespace Infrastructure::Security
