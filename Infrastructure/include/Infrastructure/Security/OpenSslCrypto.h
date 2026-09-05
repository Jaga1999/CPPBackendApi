#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Infrastructure::Security {

class OpenSslCrypto {
public:
    // Generate RSA Keypair (PEM format: privateKeyPem, publicKeyPem)
    static std::pair<std::string, std::string> generateRsaKeypair(int bits = 2048);

    // Extract RSA public components (n, e) in Base64URL format for JWK
    static std::pair<std::string, std::string> extractRsaPublicParams(std::string_view publicKeyPem);

    // RS256 Digital Signature (SHA-256 with RSA PKCS#1 v1.5)
    static std::optional<std::string> signRs256(std::string_view privateKeyPem, std::string_view message);
    static bool verifyRs256(std::string_view publicKeyPem, std::string_view message, std::string_view signatureBase64Url);

    // Base64URL RFC 7515 (no padding, url-safe chars)
    static std::string base64UrlEncode(std::string_view data);
    static std::string base64UrlEncode(const uint8_t* data, size_t length);
    static std::optional<std::string> base64UrlDecode(std::string_view base64Url);

    // Cryptographic Hashing
    static std::string sha256Hex(std::string_view data);
    static std::vector<uint8_t> sha256Raw(std::string_view data);

    // Cryptographically Secure Random Data
    static std::vector<uint8_t> randomBytes(size_t length);
    static std::string generateSecureToken(size_t byteCount = 32);
    static std::string generateUuidV4();

    // Constant-Time Memory Comparison (timing attack defense)
    static bool constantTimeCompare(std::string_view a, std::string_view b) noexcept;

    // Secure Password Hashing (PBKDF2-HMAC-SHA256 with 32-byte salt, 100,000 iterations)
    static std::string hashPassword(std::string_view password);
    static bool verifyPassword(std::string_view password, std::string_view storedHash);
};

} // namespace Infrastructure::Security
