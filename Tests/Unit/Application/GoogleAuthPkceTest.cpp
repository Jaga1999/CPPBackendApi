#include "TestHarness.h"
#include "Infrastructure/Security/GoogleAuthService.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include "Application/DTOs/GoogleAuthDtos.h"
#include "Application/DTOs/AuthDtos.h"
#include "Application/UseCases/AuthUseCases.h"
#include "Domain/Repositories/IUserRepository.h"
#include "Domain/Repositories/ISessionRepository.h"
#include "Domain/Repositories/IAuditLogRepository.h"
#include "Domain/Repositories/ICacheRepository.h"
#include "Infrastructure/Security/OpenSslCryptoAdapters.h"
#include "Infrastructure/Security/RsaKeyManager.h"
#include "Infrastructure/Security/JwtService.h"
#include <algorithm>
#include <memory>
#include <unordered_map>

namespace {

// In-Memory Mock Cache Repository for PKCE testing
class InMemoryCacheRepo : public Domain::Repositories::ICacheRepository {
public:
    std::unordered_map<std::string, Domain::Entities::CacheEntry> store;

    std::optional<Domain::Entities::CacheEntry> get(std::string_view key) override {
        auto it = store.find(std::string(key));
        if (it != store.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void set(std::string_view key, std::string_view value, int ttlSeconds) override {
        auto now = std::chrono::system_clock::now();
        store[std::string(key)] = Domain::Entities::CacheEntry{
            .key = std::string(key),
            .value = std::string(value),
            .ttlSeconds = ttlSeconds,
            .createdAt = now,
            .expiresAt = now + std::chrono::seconds(ttlSeconds)
        };
    }

    bool remove(std::string_view key) override {
        return store.erase(std::string(key)) > 0;
    }

    size_t cleanupExpired() override {
        size_t count = 0;
        for (auto it = store.begin(); it != store.end();) {
            if (it->second.isExpired()) {
                it = store.erase(it);
                ++count;
            } else {
                ++it;
            }
        }
        return count;
    }
};

// In-Memory Mock User Repository
class InMemoryUserRepo : public Domain::Repositories::IUserRepository {
public:
    std::unordered_map<std::string, Domain::Entities::User> usersById;
    std::unordered_map<std::string, std::string> emailToId;
    std::unordered_map<std::string, std::string> googleIdToId;
    int nextId{1};

    std::optional<Domain::Entities::User> findById(std::string_view id) const override {
        auto it = usersById.find(std::string(id));
        if (it != usersById.end()) return it->second;
        return std::nullopt;
    }

    std::optional<Domain::Entities::User> findByEmail(std::string_view email) const override {
        auto it = emailToId.find(std::string(email));
        if (it != emailToId.end()) return findById(it->second);
        return std::nullopt;
    }

    std::optional<Domain::Entities::User> findByGoogleId(std::string_view googleId) const override {
        auto it = googleIdToId.find(std::string(googleId));
        if (it != googleIdToId.end()) return findById(it->second);
        return std::nullopt;
    }

    Domain::Entities::User create(Domain::Entities::User user) override {
        user.id = "user-" + std::to_string(nextId++);
        usersById[user.id] = user;
        emailToId[user.email] = user.id;
        if (user.googleId.has_value()) {
            googleIdToId[*user.googleId] = user.id;
        }
        return user;
    }

    bool update(const Domain::Entities::User& user) override {
        usersById[user.id] = user;
        return true;
    }

    bool linkGoogleAccount(std::string_view userId, std::string_view googleId, std::string_view avatarUrl) override {
        auto it = usersById.find(std::string(userId));
        if (it == usersById.end()) return false;
        it->second.googleId = std::string(googleId);
        if (!avatarUrl.empty()) it->second.avatarUrl = std::string(avatarUrl);
        if (it->second.authProvider == "local") {
            it->second.authProvider = "local+google";
        }
        googleIdToId[std::string(googleId)] = it->second.id;
        return true;
    }

    bool setPassword(std::string_view userId, std::string_view passwordHash) override {
        auto it = usersById.find(std::string(userId));
        if (it == usersById.end()) return false;
        it->second.passwordHash = std::string(passwordHash);
        if (it->second.authProvider == "google") {
            it->second.authProvider = "google+local";
        }
        return true;
    }

    void incrementFailedAttempts(std::string_view) override {}
    void resetFailedAttempts(std::string_view) override {}
    void lockAccount(std::string_view, std::chrono::system_clock::time_point) override {}
};

// In-Memory Mock Session Repository
class InMemorySessionRepo : public Domain::Repositories::ISessionRepository {
public:
    std::unordered_map<std::string, Domain::Entities::Session> sessions;
    int nextId{1};

    Domain::Entities::Session createSession(Domain::Entities::Session session) override {
        session.id = "session-" + std::to_string(nextId++);
        sessions[session.id] = session;
        return session;
    }

    std::optional<Domain::Entities::Session> findById(std::string_view id) const override {
        auto it = sessions.find(std::string(id));
        if (it != sessions.end()) return it->second;
        return std::nullopt;
    }

    std::optional<Domain::Entities::Session> findByJti(std::string_view jti) const override {
        for (const auto& [_, s] : sessions) {
            if (s.jti == jti) return s;
        }
        return std::nullopt;
    }

    std::optional<Domain::Entities::Session> findByRefreshTokenHash(std::string_view hash) override {
        for (const auto& [_, s] : sessions) {
            if (s.refreshTokenHash == hash) return s;
        }
        return std::nullopt;
    }

    std::vector<Domain::Entities::Session> findUserSessions(std::string_view userId, bool includeRevoked) const override {
        std::vector<Domain::Entities::Session> res;
        for (const auto& [_, s] : sessions) {
            if (s.userId == userId && (includeRevoked || s.isActive())) res.push_back(s);
        }
        return res;
    }

    std::vector<Domain::Entities::Session> findAllSessions(
        int, int,
        std::optional<std::string_view>,
        std::optional<std::string_view>,
        std::optional<std::string_view>
    ) const override {
        std::vector<Domain::Entities::Session> res;
        for (const auto& [_, s] : sessions) res.push_back(s);
        return res;
    }

    bool updateRefreshToken(
        std::string_view sessionId,
        std::string_view newHash,
        std::string_view newJti,
        std::chrono::system_clock::time_point
    ) override {
        auto it = sessions.find(std::string(sessionId));
        if (it != sessions.end()) {
            it->second.refreshTokenHash = std::string(newHash);
            it->second.jti = std::string(newJti);
            return true;
        }
        return false;
    }

    bool revokeSession(std::string_view id, std::string_view) override {
        auto it = sessions.find(std::string(id));
        if (it != sessions.end()) {
            it->second.revokedAt = std::chrono::system_clock::now();
            return true;
        }
        return false;
    }

    size_t revokeAllUserSessions(std::string_view userId, std::string_view) override {
        size_t count = 0;
        for (auto& [_, s] : sessions) {
            if (s.userId == userId && s.isActive()) {
                s.revokedAt = std::chrono::system_clock::now();
                ++count;
            }
        }
        return count;
    }

    bool isTokenRevoked(std::string_view) const override { return false; }
    void recordTokenRevocation(std::string_view, std::string_view, std::chrono::system_clock::time_point) override {}
    size_t cleanupExpiredRevocations(std::chrono::system_clock::time_point) override { return 0; }
};

// In-Memory Mock Audit Log
class InMemoryAuditRepo : public Domain::Repositories::IAuditLogRepository {
public:
    std::vector<Domain::Entities::AuditLog> logs;
    void record(const Domain::Entities::AuditLog& log) override {
        logs.push_back(log);
    }
    std::vector<Domain::Entities::AuditLog> findLogs(
        std::optional<std::string_view>,
        std::optional<std::string_view>,
        int, int
    ) const override {
        return logs;
    }
};

// Mock Google Auth Service
class MockGoogleAuthService : public Application::Security::IGoogleAuthService {
public:
    std::string testVerifier{"dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"};
    std::string testChallenge{"E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"};
    mutable std::string lastCodeVerifierReceived;

    std::string getAuthorizationUrl(std::string_view state, std::string_view codeChallenge) const override {
        std::string url = "https://accounts.google.com/o/oauth2/v2/auth?client_id=mock-client-id";
        if (!state.empty()) url += "&state=" + std::string(state);
        if (!codeChallenge.empty()) {
            url += "&code_challenge=" + std::string(codeChallenge);
            url += "&code_challenge_method=S256";
        }
        return url;
    }

    Domain::Common::Result<Application::Security::GoogleUserInfo> verifyIdToken(std::string_view) const override {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::ok(
            Application::Security::GoogleUserInfo{
                .googleId = "google-sub-999888",
                .email = "googleuser@example.com",
                .name = "Google Tester",
                .picture = "https://lh3.googleusercontent.com/avatar.jpg",
                .emailVerified = true
            }
        );
    }

    Domain::Common::Result<Application::Security::GoogleUserInfo> exchangeAuthCode(
        std::string_view code,
        std::string_view codeVerifier
    ) const override {
        lastCodeVerifierReceived = std::string(codeVerifier);
        if (code == "valid-auth-code") {
            return Domain::Common::Result<Application::Security::GoogleUserInfo>::ok(
                Application::Security::GoogleUserInfo{
                    .googleId = "google-sub-999888",
                    .email = "googleuser@example.com",
                    .name = "Google Tester",
                    .picture = "https://lh3.googleusercontent.com/avatar.jpg",
                    .emailVerified = true
                }
            );
        }
        if (code == "link-test-code") {
            return Domain::Common::Result<Application::Security::GoogleUserInfo>::ok(
                Application::Security::GoogleUserInfo{
                    .googleId = "google-sub-111222",
                    .email = "existinguser@example.com",
                    .name = "Existing User",
                    .picture = "https://lh3.googleusercontent.com/avatar2.jpg",
                    .emailVerified = true
                }
            );
        }
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Invalid authorization code", .statusCode = 400}
        );
    }

    std::string generateCodeVerifier() const override {
        return testVerifier;
    }

    std::string generateCodeChallenge(std::string_view verifier) const override {
        auto digest = Infrastructure::Security::OpenSslCrypto::sha256Raw(verifier);
        return Infrastructure::Security::OpenSslCrypto::base64UrlEncode(digest.data(), digest.size());
    }
};

} // anonymous namespace

// =========================================================================
// 1. RFC 7636 PKCE Specification Tests
// =========================================================================

TEST_CASE("Security::PKCE", "CodeVerifierFormatAndEntropy") {
    Infrastructure::Security::GoogleAuthService service(
        "test-client.apps.googleusercontent.com",
        "test-secret",
        "http://localhost:8080/api/v1/auth/google/callback"
    );

    std::string v1 = service.generateCodeVerifier();
    std::string v2 = service.generateCodeVerifier();

    EXPECT_EQ(v1.length(), 64);
    EXPECT_EQ(v2.length(), 64);
    EXPECT_NE(v1, v2);

    auto isUnreserved = [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~';
    };

    for (char c : v1) {
        EXPECT_TRUE(isUnreserved(c));
    }
    for (char c : v2) {
        EXPECT_TRUE(isUnreserved(c));
    }
}

TEST_CASE("Security::PKCE", "Rfc7636AppendixBTestVector") {
    Infrastructure::Security::GoogleAuthService service("test-id", "test-secret", "http://localhost/callback");

    std::string verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    std::string expectedChallenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

    std::string actualChallenge = service.generateCodeChallenge(verifier);
    EXPECT_EQ(actualChallenge, expectedChallenge);
    EXPECT_EQ(actualChallenge.length(), 43);
}

TEST_CASE("Security::PKCE", "AuthorizationUrlContainsPkceChallenge") {
    Infrastructure::Security::GoogleAuthService service(
        "842447367765-test.apps.googleusercontent.com",
        "test-secret",
        "http://localhost:8080/api/v1/auth/google/callback"
    );

    std::string state = "csrf-security-state-token";
    std::string challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

    std::string authUrl = service.getAuthorizationUrl(state, challenge);

    EXPECT_TRUE(authUrl.find("code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM") != std::string::npos);
    EXPECT_TRUE(authUrl.find("code_challenge_method=S256") != std::string::npos);
    EXPECT_TRUE(authUrl.find("state=csrf-security-state-token") != std::string::npos);
    EXPECT_TRUE(authUrl.find("response_type=code") != std::string::npos);
    EXPECT_TRUE(authUrl.find("access_type=offline") != std::string::npos);
}

// =========================================================================
// 2. AuthUseCases PKCE State Cache & Exchange Tests
// =========================================================================

TEST_CASE("Application::UseCases", "GetGoogleAuthUrlGeneratesAndCachesPkce") {
    auto userRepo = std::make_shared<InMemoryUserRepo>();
    auto sessionRepo = std::make_shared<InMemorySessionRepo>();
    auto auditRepo = std::make_shared<InMemoryAuditRepo>();
    auto cacheRepo = std::make_shared<InMemoryCacheRepo>();
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-key");
    auto jwtService = std::make_shared<Infrastructure::Security::JwtService>(keyMgr, "test-iss", "test-aud");
    auto pwdHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
    auto tokenGen = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
    auto mockGoogle = std::make_shared<MockGoogleAuthService>();

    Application::UseCases::AuthUseCases useCases(
        userRepo, sessionRepo, auditRepo, jwtService, pwdHasher, tokenGen, mockGoogle, cacheRepo
    );

    auto res = useCases.getGoogleAuthUrl("my-state-token-123");
    EXPECT_TRUE(res.isOk());
    const auto& data = res.value().data;
    EXPECT_TRUE(data.has_value());
    EXPECT_EQ(data->state, "my-state-token-123");
    EXPECT_EQ(data->codeVerifier, "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
    EXPECT_EQ(data->codeChallenge, "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM");
    EXPECT_TRUE(data->authUrl.find("code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM") != std::string::npos);

    auto cached = cacheRepo->get("pkce:state:my-state-token-123");
    EXPECT_TRUE(cached.has_value());
    EXPECT_EQ(cached->value, "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
}

TEST_CASE("Application::UseCases", "GoogleLoginResolvesAndConsumesCachedPkceVerifier") {
    auto userRepo = std::make_shared<InMemoryUserRepo>();
    auto sessionRepo = std::make_shared<InMemorySessionRepo>();
    auto auditRepo = std::make_shared<InMemoryAuditRepo>();
    auto cacheRepo = std::make_shared<InMemoryCacheRepo>();
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-key");
    auto jwtService = std::make_shared<Infrastructure::Security::JwtService>(keyMgr, "test-iss", "test-aud");
    auto pwdHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
    auto tokenGen = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
    auto mockGoogle = std::make_shared<MockGoogleAuthService>();

    Application::UseCases::AuthUseCases useCases(
        userRepo, sessionRepo, auditRepo, jwtService, pwdHasher, tokenGen, mockGoogle, cacheRepo
    );

    auto urlRes = useCases.getGoogleAuthUrl("state-abc-789");
    EXPECT_TRUE(urlRes.isOk());

    Application::DTOs::GoogleLoginRequest req{
        .idToken = "",
        .code = "valid-auth-code",
        .codeVerifier = "",
        .state = "state-abc-789"
    };

    auto loginRes = useCases.loginWithGoogle(req, "127.0.0.1", "Chrome");
    EXPECT_TRUE(loginRes.isOk());

    EXPECT_EQ(mockGoogle->lastCodeVerifierReceived, "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");

    auto cachedAfter = cacheRepo->get("pkce:state:state-abc-789");
    EXPECT_FALSE(cachedAfter.has_value());

    const auto& tokenData = loginRes.value().data;
    EXPECT_TRUE(tokenData.has_value());
    EXPECT_EQ(tokenData->user.email, "googleuser@example.com");
    EXPECT_EQ(tokenData->user.authProvider, "google");
    EXPECT_TRUE(tokenData->user.googleLinked);
}

// =========================================================================
// 3. Bidirectional Account Linking & Password Setting Tests
// =========================================================================

TEST_CASE("Application::UseCases", "BidirectionalAccountLinkingLocalToGoogle") {
    auto userRepo = std::make_shared<InMemoryUserRepo>();
    auto sessionRepo = std::make_shared<InMemorySessionRepo>();
    auto auditRepo = std::make_shared<InMemoryAuditRepo>();
    auto cacheRepo = std::make_shared<InMemoryCacheRepo>();
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-key");
    auto jwtService = std::make_shared<Infrastructure::Security::JwtService>(keyMgr, "test-iss", "test-aud");
    auto pwdHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
    auto tokenGen = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
    auto mockGoogle = std::make_shared<MockGoogleAuthService>();

    Application::UseCases::AuthUseCases useCases(
        userRepo, sessionRepo, auditRepo, jwtService, pwdHasher, tokenGen, mockGoogle, cacheRepo
    );

    Application::DTOs::RegisterRequest regReq{
        .email = "existinguser@example.com",
        .password = "SecurePassword123!",
        .role = "user"
    };
    auto regRes = useCases.registerUser(regReq);
    EXPECT_TRUE(regRes.isOk());
    std::string existingUserId = regRes.value().data->id;

    Application::DTOs::GoogleLoginRequest googleReq{
        .idToken = "",
        .code = "link-test-code",
        .codeVerifier = "test-verifier-client"
    };
    auto googleLoginRes = useCases.loginWithGoogle(googleReq, "127.0.0.1", "Firefox");
    EXPECT_TRUE(googleLoginRes.isOk());

    const auto& tokenData = googleLoginRes.value().data;
    EXPECT_TRUE(tokenData.has_value());
    EXPECT_EQ(tokenData->user.id, existingUserId);
    EXPECT_EQ(tokenData->user.email, "existinguser@example.com");
    EXPECT_EQ(tokenData->user.authProvider, "local+google");
    EXPECT_TRUE(tokenData->user.googleLinked);

    Application::DTOs::LoginRequest localLoginReq{
        .email = "existinguser@example.com",
        .password = "SecurePassword123!"
    };
    auto localLoginRes = useCases.login(localLoginReq, "127.0.0.1", "Firefox", "Laptop");
    EXPECT_TRUE(localLoginRes.isOk());
    EXPECT_EQ(localLoginRes.value().data->user.id, existingUserId);
    EXPECT_TRUE(localLoginRes.value().data->user.googleLinked);
}

TEST_CASE("Application::UseCases", "SetPasswordAllowsGoogleUserToLoginLocally") {
    auto userRepo = std::make_shared<InMemoryUserRepo>();
    auto sessionRepo = std::make_shared<InMemorySessionRepo>();
    auto auditRepo = std::make_shared<InMemoryAuditRepo>();
    auto cacheRepo = std::make_shared<InMemoryCacheRepo>();
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("test-key");
    auto jwtService = std::make_shared<Infrastructure::Security::JwtService>(keyMgr, "test-iss", "test-aud");
    auto pwdHasher = std::make_shared<Infrastructure::Security::OpenSslPasswordHasher>();
    auto tokenGen = std::make_shared<Infrastructure::Security::OpenSslTokenGenerator>();
    auto mockGoogle = std::make_shared<MockGoogleAuthService>();

    Application::UseCases::AuthUseCases useCases(
        userRepo, sessionRepo, auditRepo, jwtService, pwdHasher, tokenGen, mockGoogle, cacheRepo
    );

    Application::DTOs::GoogleLoginRequest googleReq{
        .idToken = "",
        .code = "valid-auth-code",
        .codeVerifier = "client-pkce-verifier-123"
    };
    auto googleLoginRes = useCases.loginWithGoogle(googleReq, "127.0.0.1", "Chrome");
    EXPECT_TRUE(googleLoginRes.isOk());
    std::string userId = googleLoginRes.value().data->user.id;

    Application::DTOs::LoginRequest localBefore{
        .email = "googleuser@example.com",
        .password = "SomePassword123!"
    };
    auto failRes = useCases.login(localBefore, "127.0.0.1", "Chrome", "PC");
    EXPECT_TRUE(failRes.isFailure());

    Application::DTOs::SetPasswordRequest setPwdReq{.password = "NewLocalSecretPass456!"};
    auto setPwdRes = useCases.setPassword(userId, setPwdReq);
    EXPECT_TRUE(setPwdRes.isOk());

    Application::DTOs::LoginRequest localAfter{
        .email = "googleuser@example.com",
        .password = "NewLocalSecretPass456!"
    };
    auto successRes = useCases.login(localAfter, "127.0.0.1", "Chrome", "PC");
    EXPECT_TRUE(successRes.isOk());
    EXPECT_EQ(successRes.value().data->user.id, userId);
}
