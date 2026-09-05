#include "TestHarness.h"
#include "Infrastructure/Security/RsaKeyManager.h"
#include <crow/json.h>

TEST_CASE("Infrastructure::Security", "RsaKeyManagerGeneratesAndExportsJwks") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-key-2026");

    EXPECT_EQ(keyMgr->getActiveSigningKey().kid, "test-key-2026");

    std::string jwksJson = keyMgr->getJwksJson();
    EXPECT_FALSE(jwksJson.empty());

    auto parsed = crow::json::load(jwksJson);
    EXPECT_TRUE(parsed);
    EXPECT_TRUE(parsed.has("keys"));
    EXPECT_TRUE(parsed["keys"].size() >= 1);

    const auto& key = parsed["keys"][0];
    EXPECT_EQ(key["kty"].s(), "RSA");
    EXPECT_EQ(key["use"].s(), "sig");
    EXPECT_EQ(key["alg"].s(), "RS256");
    EXPECT_EQ(key["kid"].s(), "test-key-2026");
    EXPECT_TRUE(key.has("n"));
    EXPECT_TRUE(key.has("e"));
}

TEST_CASE("Infrastructure::Security", "RsaKeyManagerRotateKey") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-key-v1");
    EXPECT_EQ(keyMgr->getActiveSigningKey().kid, "test-key-v1");

    keyMgr->rotateKey("test-key-v2");
    EXPECT_EQ(keyMgr->getActiveSigningKey().kid, "test-key-v2");

    std::string jwksJson = keyMgr->getJwksJson();
    auto parsed = crow::json::load(jwksJson);
    EXPECT_TRUE(parsed["keys"].size() >= 2);
}
