#include "TestHarness.h"
#include "Infrastructure/Security/JwtService.h"
#include "Infrastructure/Security/RsaKeyManager.h"

TEST_CASE("Infrastructure::Security", "JwtServiceCreateAndValidateToken") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-jwt-key");
    Infrastructure::Security::JwtService jwtService(keyMgr, "test-issuer", "test-audience");

    std::string token = jwtService.createAccessToken(
        "usr-uuid-42",
        "user",
        "sess-uuid-99",
        "jti-uuid-101",
        std::chrono::seconds(60)
    );

    EXPECT_FALSE(token.empty());

    // Verify token parts (header.payload.signature)
    size_t firstDot = token.find('.');
    size_t secondDot = token.rfind('.');
    EXPECT_TRUE(firstDot != std::string::npos);
    EXPECT_TRUE(secondDot != std::string::npos);
    EXPECT_NE(firstDot, secondDot);

    // Validate token
    auto claimsRes = jwtService.validateAccessToken(token);
    EXPECT_TRUE(claimsRes.isSuccess());

    const auto& claims = claimsRes.value();
    EXPECT_EQ(claims.sub, "usr-uuid-42");
    EXPECT_EQ(claims.sid, "sess-uuid-99");
    EXPECT_EQ(claims.jti, "jti-uuid-101");
    EXPECT_EQ(claims.role, "user");
}

TEST_CASE("Infrastructure::Security", "JwtServiceRejectsExpiredToken") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-jwt-key");
    Infrastructure::Security::JwtService jwtService(keyMgr, "test-issuer", "test-audience");

    // Token with -5 seconds TTL (already expired)
    std::string expiredToken = jwtService.createAccessToken(
        "usr-expired",
        "user",
        "sess-expired",
        "jti-expired",
        std::chrono::seconds(-5)
    );

    auto claimsRes = jwtService.validateAccessToken(expiredToken);
    EXPECT_FALSE(claimsRes.isSuccess());
}
