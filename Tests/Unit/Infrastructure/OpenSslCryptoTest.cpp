#include "TestHarness.h"
#include "Infrastructure/Security/OpenSslCrypto.h"

TEST_CASE("Infrastructure::Crypto", "Base64UrlEncodeAndDecodeRoundtrip") {
    std::string original = "Hello World! Standard C++20 Base64URL test.";
    std::string encoded = Infrastructure::Security::OpenSslCrypto::base64UrlEncode(original);
    
    // Ensure no '=' padding
    EXPECT_TRUE(encoded.find('=') == std::string::npos);
    EXPECT_TRUE(encoded.find('+') == std::string::npos);
    EXPECT_TRUE(encoded.find('/') == std::string::npos);

    auto decodedOpt = Infrastructure::Security::OpenSslCrypto::base64UrlDecode(encoded);
    EXPECT_TRUE(decodedOpt.has_value());
    EXPECT_EQ(original, *decodedOpt);
}

TEST_CASE("Infrastructure::Crypto", "Sha256HashDeterministic") {
    std::string input = "deterministic_refresh_token_string";
    std::string hash1 = Infrastructure::Security::OpenSslCrypto::sha256Hex(input);
    std::string hash2 = Infrastructure::Security::OpenSslCrypto::sha256Hex(input);

    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash1.length(), 64); // 32 bytes hex encoded
}

TEST_CASE("Infrastructure::Crypto", "Pbkdf2PasswordHashAndVerify") {
    std::string password = "SuperSecretPassword2026!";
    std::string hash = Infrastructure::Security::OpenSslCrypto::hashPassword(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_TRUE(hash.find('$') != std::string::npos);

    // Correct password matches
    EXPECT_TRUE(Infrastructure::Security::OpenSslCrypto::verifyPassword(password, hash));

    // Wrong password fails
    EXPECT_FALSE(Infrastructure::Security::OpenSslCrypto::verifyPassword("WrongPassword!", hash));
}

TEST_CASE("Infrastructure::Crypto", "SecureRandomTokenGeneration") {
    std::string token1 = Infrastructure::Security::OpenSslCrypto::generateSecureToken(32);
    std::string token2 = Infrastructure::Security::OpenSslCrypto::generateSecureToken(32);

    EXPECT_FALSE(token1.empty());
    EXPECT_FALSE(token2.empty());
    EXPECT_NE(token1, token2);
}
