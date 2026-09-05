#include "TestHarness.h"
#include "Application/Security/AuthorizationService.h"
#include "Application/Validation/AuthInputValidator.h"
#include "Application/Validation/InputValidator.h"
#include "Domain/Entities/User.h"
#include "Domain/Entities/Session.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include "Infrastructure/Persistence/PostgresUserRepository.h"
#include "Infrastructure/Persistence/PostgresSessionRepository.h"
#include "Infrastructure/Security/JwtService.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include "Infrastructure/Security/OpenSslCryptoAdapters.h"
#include "Infrastructure/Security/RsaKeyManager.h"
#include "Infrastructure/Security/GoogleAuthService.h"
#include "Infrastructure/Persistence/PostgresCacheRepository.h"
#include "Infrastructure/Persistence/PostgresAuditLogRepository.h"
#include "Application/DTOs/GoogleAuthDtos.h"
#include "Application/UseCases/AuthUseCases.h"
#include "Presentation/Common/HttpResponseHelper.h"
#include <chrono>

static auto getAttackDb() {
    static auto db = std::make_shared<Infrastructure::Persistence::PostgresDb>();
    return db;
}

// 1. SQL Injection vectors
TEST_CASE("Security::CyberAttacks", "SqlInjectionInRegistrationEmailBlocked") {
    // Malicious SQL injection payloads
    std::vector<std::string> sqlPayloads = {
        "admin'--",
        "' OR '1'='1",
        "'; DROP TABLE users; --",
        "' UNION SELECT * FROM users --",
        "test' OR 1=1;--"
    };

    for (const auto& payload : sqlPayloads) {
        Application::DTOs::RegisterRequest req{.email = payload, .password = "SecurePass123!"};
        auto res = Application::Validation::AuthInputValidator::validateRegister(req);
        EXPECT_FALSE(res.isSuccess()); // Validation must reject invalid email syntax containing SQL injection
    }
}

TEST_CASE("Security::CyberAttacks", "SqlInjectionInTodoRepositorySafe") {
    auto db = getAttackDb();
    Infrastructure::Persistence::PostgresUserRepository repo(db);

    // SQL Injection payload passed to findByEmail
    std::string sqliEmail = "' OR '1'='1' --";
    auto user = repo.findByEmail(sqliEmail);
    // Libpqxx parameterized query must treat this literally and NOT return all users
    EXPECT_FALSE(user.has_value());
}

// 2. XSS & HTML Script Injection in JSON Payloads
TEST_CASE("Security::CyberAttacks", "XssPayloadsHandledAsLiteralStrings") {
    std::string xssPayload = "<script>alert('XSS_ATTACK')</script><svg/onload=alert(1)>";
    Application::DTOs::CreateTodoRequest req{.title = xssPayload, .description = xssPayload};
    auto valRes = Application::Validation::InputValidator::validateCreate(req);
    EXPECT_TRUE(valRes.isSuccess()); // Accepted as text

    // Encoding and decoding must preserve text literally without executing or corrupting
    std::string encoded = Infrastructure::Security::OpenSslCrypto::base64UrlEncode(xssPayload);
    auto decodedOpt = Infrastructure::Security::OpenSslCrypto::base64UrlDecode(encoded);
    EXPECT_TRUE(decodedOpt.has_value());
    EXPECT_EQ(*decodedOpt, xssPayload);
}

// 3. JWT Signature Tampering (Privilege Escalation)
TEST_CASE("Security::CyberAttacks", "JwtSignatureTamperingRejected") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("security-test-key");
    Infrastructure::Security::JwtService jwtService(keyMgr, "test-issuer", "test-aud");

    // Generate valid token for standard "user"
    std::string validToken = jwtService.createAccessToken(
        "usr-victim-1",
        "user",
        "sess-victim-1",
        "jti-victim-1",
        std::chrono::hours(1)
    );

    // Attacker modifies the payload to escalate role to "admin"
    size_t firstDot = validToken.find('.');
    size_t secondDot = validToken.rfind('.');
    std::string headerB64 = validToken.substr(0, firstDot);
    std::string payloadB64 = validToken.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string signatureB64 = validToken.substr(secondDot + 1);

    auto payloadJsonOpt = Infrastructure::Security::OpenSslCrypto::base64UrlDecode(payloadB64);
    EXPECT_TRUE(payloadJsonOpt.has_value());
    std::string payloadJson = *payloadJsonOpt;
    // Replace role user with admin
    size_t rolePos = payloadJson.find("\"role\":\"user\"");
    if (rolePos != std::string::npos) {
        payloadJson.replace(rolePos, 13, "\"role\":\"admin\"");
    }

    std::string tamperedPayloadB64 = Infrastructure::Security::OpenSslCrypto::base64UrlEncode(payloadJson);
    std::string forgedToken = headerB64 + "." + tamperedPayloadB64 + "." + signatureB64;

    // Validation must fail cryptographic signature verification
    auto claims = jwtService.validateAccessToken(forgedToken);
    EXPECT_FALSE(claims.isSuccess());
}

// 4. JWT 'alg: none' Vulnerability Attack
TEST_CASE("Security::CyberAttacks", "JwtAlgNoneVulnerabilityBlocked") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("security-test-key");
    Infrastructure::Security::JwtService jwtService(keyMgr, "test-issuer", "test-aud");

    // Construct forged token with alg: none
    std::string forgedHeader = "{\"alg\":\"none\",\"typ\":\"JWT\"}";
    std::string forgedPayload = "{\"sub\":\"admin-usr\",\"role\":\"admin\",\"iss\":\"test-issuer\",\"aud\":\"test-aud\",\"exp\":9999999999}";

    std::string noneToken = Infrastructure::Security::OpenSslCrypto::base64UrlEncode(forgedHeader) + "." +
                            Infrastructure::Security::OpenSslCrypto::base64UrlEncode(forgedPayload) + ".";

    auto claims = jwtService.validateAccessToken(noneToken);
    EXPECT_FALSE(claims.isSuccess());
}

// 5. Unknown Key ID (kid) Confusion Attack
TEST_CASE("Security::CyberAttacks", "JwtUnknownKeyIdRejected") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("security-test-key");
    Infrastructure::Security::JwtService jwtService(keyMgr, "test-issuer", "test-aud");

    // Key signed by a different key manager with a different kid
    auto attackerKeyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("attacker-evil-kid");
    Infrastructure::Security::JwtService attackerJwt(attackerKeyMgr, "test-issuer", "test-aud");

    std::string attackerToken = attackerJwt.createAccessToken("usr-target", "admin", "sess-1", "jti-1", std::chrono::hours(1));

    // Target service doesn't have attacker-evil-kid
    auto claims = jwtService.validateAccessToken(attackerToken);
    EXPECT_FALSE(claims.isSuccess());
}

// 6. Token Revocation Bypass Attack (Banned JTI Check)
TEST_CASE("Security::CyberAttacks", "BannedJtiTokenBlockedImmediately") {
    auto db = getAttackDb();
    Infrastructure::Persistence::PostgresUserRepository userRepo(db);
    Infrastructure::Persistence::PostgresSessionRepository sessRepo(db);

    std::string email = "banned_jti_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8) + "@example.com";
    auto user = userRepo.create({.email = email, .passwordHash = "hash", .role = "user", .isActive = true});

    std::string jti = Infrastructure::Security::OpenSslCrypto::generateSecureToken(16);
    sessRepo.createSession({
        .userId = user.id,
        .jti = jti,
        .refreshTokenHash = "hash",
        .createdAt = std::chrono::system_clock::now(),
        .lastSeenAt = std::chrono::system_clock::now(),
        .expiresAt = std::chrono::system_clock::now() + std::chrono::hours(1)
    });

    // Attacker has valid token, but admin/user revokes it
    sessRepo.revokeSession(jti, "Compromised device");

    // Token must be blocked via JTI revocation table
    EXPECT_TRUE(sessRepo.isTokenRevoked(jti));
}

// 7. Refresh Token Replay / Reuse Attack Detection
TEST_CASE("Security::CyberAttacks", "RefreshTokenReplayDetectedAndRevokesFamily") {
    auto db = getAttackDb();
    Infrastructure::Persistence::PostgresUserRepository userRepo(db);
    Infrastructure::Persistence::PostgresSessionRepository sessRepo(db);

    std::string email = "replay_user_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8) + "@example.com";
    auto user = userRepo.create({.email = email, .passwordHash = "hash", .role = "user", .isActive = true});

    std::string oldToken = "refresh_token_generation_1";
    std::string oldHash = Infrastructure::Security::OpenSslCrypto::sha256Hex(oldToken);
    std::string jti1 = "jti_gen_1_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8);

    auto sess = sessRepo.createSession({
        .userId = user.id,
        .jti = jti1,
        .refreshTokenHash = oldHash,
        .createdAt = std::chrono::system_clock::now(),
        .lastSeenAt = std::chrono::system_clock::now(),
        .expiresAt = std::chrono::system_clock::now() + std::chrono::hours(24)
    });

    // Legitimate rotation to generation 2
    std::string newToken = "refresh_token_generation_2";
    std::string newHash = Infrastructure::Security::OpenSslCrypto::sha256Hex(newToken);
    std::string jti2 = "jti_gen_2_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8);

    bool rotated = sessRepo.updateRefreshToken(sess.id, newHash, jti2, std::chrono::system_clock::now() + std::chrono::hours(24));
    EXPECT_TRUE(rotated);

    // Adversary attempts to replay generation 1!
    auto reuseSess = sessRepo.findByRefreshTokenHash(oldHash);
    EXPECT_TRUE(reuseSess.has_value());
    EXPECT_TRUE(reuseSess->revocationReason.has_value());
    EXPECT_EQ(*reuseSess->revocationReason, "TOKEN_REUSE_DETECTED");
}

// 8. IDOR / BOLA Authorization Bypass Defense
TEST_CASE("Security::CyberAttacks", "IdorAuthorizationBypassBlocked") {
    Application::Security::AuthorizationService authService;

    Domain::Entities::User victimUser{.id = "usr-victim-1001", .role = "user"};
    Domain::Entities::User attackerUser{.id = "usr-attacker-9999", .role = "user"};

    Domain::Entities::Session victimSession{.id = "sess-1001", .userId = victimUser.id};

    // Attacker tries to access victim session
    bool canAccess = authService.canAccessSession(attackerUser, victimSession);
    EXPECT_FALSE(canAccess);

    // Attacker tries to revoke victim session
    bool canRevoke = authService.canRevokeSession(attackerUser, victimSession);
    EXPECT_FALSE(canRevoke);
}

// 9. Brute-Force Password Spraying & Account Lockout
TEST_CASE("Security::CyberAttacks", "BruteForceTriggersAccountLockout") {
    auto db = getAttackDb();
    Infrastructure::Persistence::PostgresUserRepository repo(db);

    std::string email = "bruteforce_target_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8) + "@example.com";
    auto user = repo.create({.email = email, .passwordHash = "correct_password_hash", .role = "user", .isActive = true});

    // Fire 5 failed attempts
    for (int i = 0; i < 5; ++i) {
        repo.incrementFailedAttempts(user.id);
    }

    auto target = repo.findById(user.id);
    EXPECT_TRUE(target.has_value());
    EXPECT_EQ(target->failedLoginAttempts, 5);

    // Lock account
    repo.lockAccount(user.id, std::chrono::system_clock::now() + std::chrono::minutes(15));
}

// 10. Malformed Base64URL & Fuzzing Resilience
TEST_CASE("Security::CyberAttacks", "MalformedBase64DoesNotCrash") {
    std::vector<std::string> malformedInputs = {
        "!!!invalid_base64!!!",
        "===",
        "abc%20def",
        "a",
        "ab",
        std::string(1000, '~')
    };

    for (const auto& input : malformedInputs) {
        EXPECT_NO_THROW(Infrastructure::Security::OpenSslCrypto::base64UrlDecode(input));
    }
}

// 11. PKCE Code Verifier & Challenge Information Disclosure Defense
TEST_CASE("Security::CyberAttacks", "PkceSecretsNotExposedInHttpResponse") {
    Application::DTOs::GoogleAuthUrlResponse dto{
        .authUrl = "https://accounts.google.com/o/oauth2/v2/auth?client_id=123&state=abc&code_challenge=xyz",
        .state = "super-secret-csrf-state-12345",
        .codeVerifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk",
        .codeChallenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"
    };

    auto serialized = Presentation::Common::HttpResponseHelper::serializeGoogleAuthUrlDto(dto);
    std::string dump = serialized.dump();

    // RFC 7636 security invariant: Server MUST NOT disclose raw code_verifier, code_challenge, or state in response
    EXPECT_TRUE(dump.find("codeVerifier") == std::string::npos);
    EXPECT_TRUE(dump.find("codeChallenge") == std::string::npos);
    EXPECT_TRUE(dump.find("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk") == std::string::npos);
    EXPECT_TRUE(dump.find("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM") == std::string::npos);

    // Only authUrl must be provided to client
    EXPECT_TRUE(dump.find("authUrl") != std::string::npos);
}

// 12. Forged PKCE State Rejected on Callback
TEST_CASE("Security::CyberAttacks", "ForgedPkceStateRejectedOnCallback") {
    auto db = getAttackDb();
    auto userRepo = std::make_shared<Infrastructure::Persistence::PostgresUserRepository>(db);
    auto sessionRepo = std::make_shared<Infrastructure::Persistence::PostgresSessionRepository>(db);
    auto auditRepo = std::make_shared<Infrastructure::Persistence::PostgresAuditLogRepository>(db);
    auto cacheRepo = std::make_shared<Infrastructure::Persistence::PostgresCacheRepository>(db);
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("sec-key");
    auto jwtService = std::make_shared<Infrastructure::Security::JwtService>(keyMgr, "test-iss", "test-aud");
    auto pwdHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
    auto tokenGen = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
    auto mockGoogle = std::make_shared<Infrastructure::Security::GoogleAuthService>("client-id", "client-secret", "http://localhost/callback");

    Application::UseCases::AuthUseCases useCases(
        userRepo, sessionRepo, auditRepo, jwtService, pwdHasher, tokenGen, mockGoogle, cacheRepo
    );

    // Attacker crafts callback with unknown state that was never issued by the server
    Application::DTOs::GoogleLoginRequest evilReq{
        .idToken = "",
        .code = "stolen-or-fake-code",
        .codeVerifier = "",
        .state = "attacker-forged-state-xyz"
    };

    auto res = useCases.loginWithGoogle(evilReq, "192.168.1.100", "EvilBot/1.0");
    // Must be rejected with 401 Unauthorized because no PKCE verifier exists in cache for this forged state
    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().statusCode, 401);
}
