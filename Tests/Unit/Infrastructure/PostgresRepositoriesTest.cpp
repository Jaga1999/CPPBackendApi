#include "TestHarness.h"
#include "Infrastructure/Config/EnvLoader.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include "Infrastructure/Persistence/PostgresTodoRepository.h"
#include "Infrastructure/Persistence/PostgresCacheRepository.h"
#include "Infrastructure/Persistence/PostgresMessageQueueRepository.h"
#include "Infrastructure/Persistence/PostgresDocumentRepository.h"
#include "Infrastructure/Persistence/PostgresUserRepository.h"
#include "Infrastructure/Persistence/PostgresSessionRepository.h"
#include "Infrastructure/Persistence/PostgresAuditLogRepository.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include <chrono>

static std::shared_ptr<Infrastructure::Persistence::PostgresDb> getTestDb() {
    static auto db = std::make_shared<Infrastructure::Persistence::PostgresDb>();
    return db;
}

TEST_CASE("Infrastructure::Postgres", "TodoRepositoryCrud") {
    auto db = getTestDb();
    Infrastructure::Persistence::PostgresTodoRepository repo(db);

    Domain::Entities::Todo todo{
        .title = "Unit Test Postgres Todo",
        .description = "Test Desc",
        .completed = false
    };

    auto saved = repo.save(todo);
    EXPECT_TRUE(saved.id > 0);

    auto fetched = repo.findById(saved.id);
    EXPECT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->title, "Unit Test Postgres Todo");

    auto updated = repo.update(saved.id, "Updated Postgres Todo", std::nullopt, true);
    EXPECT_TRUE(updated.has_value());
    EXPECT_EQ(updated->title, "Updated Postgres Todo");
    EXPECT_TRUE(updated->completed);

    bool removed = repo.remove(saved.id);
    EXPECT_TRUE(removed);

    auto afterDelete = repo.findById(saved.id);
    EXPECT_FALSE(afterDelete.has_value());
}

TEST_CASE("Infrastructure::Postgres", "CacheRepositorySetGetTtl") {
    auto db = getTestDb();
    Infrastructure::Persistence::PostgresCacheRepository repo(db);

    std::string key = "test:unit:cache:" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8);
    repo.set(key, "cached_data_value", 300);

    auto entry = repo.get(key);
    EXPECT_TRUE(entry.has_value());
    EXPECT_EQ(entry->value, "cached_data_value");

    bool removed = repo.remove(key);
    EXPECT_TRUE(removed);

    auto entryAfterRemove = repo.get(key);
    EXPECT_FALSE(entryAfterRemove.has_value());
}

TEST_CASE("Infrastructure::Postgres", "MessageQueuePublishPollAck") {
    auto db = getTestDb();
    Infrastructure::Persistence::PostgresMessageQueueRepository repo(db);

    std::string topic = "unit.test.queue." + Infrastructure::Security::OpenSslCrypto::generateSecureToken(6);
    std::string payload = "{\"event\":\"order_created\",\"amount\":49.99}";

    uint64_t msgId = repo.publish(topic, payload);
    EXPECT_TRUE(msgId > 0);

    auto polled = repo.pollNext(topic);
    EXPECT_TRUE(polled.has_value());
    EXPECT_EQ(polled->id, msgId);
    EXPECT_EQ(polled->topic, topic);
    EXPECT_EQ(polled->status, "PROCESSING");

    bool acked = repo.acknowledge(msgId);
    EXPECT_TRUE(acked);
}

TEST_CASE("Infrastructure::Postgres", "DocumentRepositoryJsonbContainment") {
    auto db = getTestDb();
    Infrastructure::Persistence::PostgresDocumentRepository repo(db);

    std::string collection = "unit_test_col";
    std::string uniqueTag = "tag_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8);
    std::string jsonData = "{\"name\":\"Widget\",\"price\":29.99,\"unique_tag\":\"" + uniqueTag + "\"}";

    auto inserted = repo.insert(collection, jsonData);
    EXPECT_FALSE(inserted.id.empty());

    auto fetched = repo.findById(collection, inserted.id);
    EXPECT_TRUE(fetched.has_value());

    std::string filter = "{\"unique_tag\":\"" + uniqueTag + "\"}";
    auto queried = repo.queryByFilter(collection, filter);
    EXPECT_TRUE(queried.size() == 1);
    EXPECT_EQ(queried[0].id, inserted.id);

    bool removed = repo.remove(collection, inserted.id);
    EXPECT_TRUE(removed);
}

TEST_CASE("Infrastructure::Postgres", "UserRepositoryLockoutAndFailedAttempts") {
    auto db = getTestDb();
    Infrastructure::Persistence::PostgresUserRepository repo(db);

    std::string email = "test_user_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8) + "@example.com";
    Domain::Entities::User newUser{
        .email = email,
        .passwordHash = "sample_hash",
        .role = "user",
        .isActive = true
    };

    auto created = repo.create(newUser);
    EXPECT_FALSE(created.id.empty());

    // Increment failed attempts
    repo.incrementFailedAttempts(created.id);
    repo.incrementFailedAttempts(created.id);

    auto fetched = repo.findById(created.id);
    EXPECT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->failedLoginAttempts, 2);

    // Lock account
    auto lockUntil = std::chrono::system_clock::now() + std::chrono::minutes(15);
    repo.lockAccount(created.id, lockUntil);

    // Reset failed attempts
    repo.resetFailedAttempts(created.id);
    auto afterReset = repo.findById(created.id);
    EXPECT_TRUE(afterReset.has_value());
    EXPECT_EQ(afterReset->failedLoginAttempts, 0);
}

TEST_CASE("Infrastructure::Postgres", "SessionRepositoryRevocationAndJtiCheck") {
    auto db = getTestDb();
    Infrastructure::Persistence::PostgresUserRepository userRepo(db);
    Infrastructure::Persistence::PostgresSessionRepository sessRepo(db);

    std::string email = "sess_user_" + Infrastructure::Security::OpenSslCrypto::generateSecureToken(8) + "@example.com";
    auto user = userRepo.create({.email = email, .passwordHash = "hash", .role = "user", .isActive = true});

    std::string jti = Infrastructure::Security::OpenSslCrypto::generateSecureToken(16);
    Domain::Entities::Session sess{
        .userId = user.id,
        .jti = jti,
        .refreshTokenHash = Infrastructure::Security::OpenSslCrypto::sha256Hex("refresh_token_xyz"),
        .createdAt = std::chrono::system_clock::now(),
        .lastSeenAt = std::chrono::system_clock::now(),
        .expiresAt = std::chrono::system_clock::now() + std::chrono::hours(1),
        .ipAddress = "127.0.0.1",
        .userAgent = "TestAgent",
        .deviceName = "TestLaptop",
        .clientType = "desktop"
    };

    auto createdSess = sessRepo.createSession(sess);
    EXPECT_FALSE(createdSess.id.empty());

    // Token should NOT be revoked yet
    EXPECT_FALSE(sessRepo.isTokenRevoked(jti));

    // Revoke session
    bool revoked = sessRepo.revokeSession(jti, "User manual logout");
    EXPECT_TRUE(revoked);

    // Token must now be detected as revoked
    EXPECT_TRUE(sessRepo.isTokenRevoked(jti));
}
