#include "TestHarness.h"
#include "Domain/Common/Permissions.h"
#include "Domain/Common/Result.h"
#include "Domain/Entities/AuditLog.h"
#include "Domain/Entities/CacheEntry.h"
#include "Domain/Entities/DocumentEntity.h"
#include "Domain/Entities/QueueMessage.h"
#include "Domain/Entities/Session.h"
#include "Domain/Entities/Todo.h"
#include "Domain/Entities/User.h"

TEST_CASE("Domain::Result", "SuccessResultHoldsValue") {
    Domain::Common::Result<int> res = Domain::Common::Result<int>::ok(42);
    EXPECT_TRUE(res.isOk());
    EXPECT_TRUE(res.isSuccess());
    EXPECT_FALSE(res.isFailure());
    EXPECT_EQ(res.value(), 42);
}

TEST_CASE("Domain::Result", "FailureResultHoldsError") {
    Domain::Common::DomainError err{.message = "Item not found in repository", .statusCode = 404};
    Domain::Common::Result<int> res = Domain::Common::Result<int>::err(err);
    EXPECT_FALSE(res.isOk());
    EXPECT_FALSE(res.isSuccess());
    EXPECT_TRUE(res.isFailure());
    EXPECT_EQ(res.error().statusCode, 404);
    EXPECT_EQ(res.error().message, "Item not found in repository");
}

TEST_CASE("Domain::Permissions", "RoleHierarchyAndPermissions") {
    EXPECT_TRUE(Domain::Common::roleHasPermission("admin", Domain::Common::Permission::SessionReadAll));
    EXPECT_TRUE(Domain::Common::roleHasPermission("admin", Domain::Common::Permission::SessionRevokeAll));
    EXPECT_TRUE(Domain::Common::roleHasPermission("admin", Domain::Common::Permission::SessionReadSelf));
    EXPECT_TRUE(Domain::Common::roleHasPermission("admin", Domain::Common::Permission::UserManage));

    EXPECT_FALSE(Domain::Common::roleHasPermission("user", Domain::Common::Permission::SessionReadAll));
    EXPECT_FALSE(Domain::Common::roleHasPermission("user", Domain::Common::Permission::SessionRevokeAll));
    EXPECT_TRUE(Domain::Common::roleHasPermission("user", Domain::Common::Permission::SessionReadSelf));
    EXPECT_TRUE(Domain::Common::roleHasPermission("user", Domain::Common::Permission::SessionRevokeSelf));
    EXPECT_FALSE(Domain::Common::roleHasPermission("user", Domain::Common::Permission::UserManage));

    EXPECT_EQ(Domain::Common::permissionToString(Domain::Common::Permission::SessionReadSelf), "session:read:self");
    EXPECT_EQ(Domain::Common::permissionToString(Domain::Common::Permission::UserManage), "user:manage");
}

TEST_CASE("Domain::Entities", "TodoValidationAndProperties") {
    Domain::Entities::Todo todo{
        .id = 101,
        .title = "Test Domain Todo",
        .description = "Detailed description",
        .completed = false
    };

    EXPECT_EQ(todo.id, 101);
    EXPECT_EQ(todo.title, "Test Domain Todo");
    EXPECT_FALSE(todo.completed);
}

TEST_CASE("Domain::Entities", "UserEntityInvariants") {
    Domain::Entities::User user{
        .id = "usr-12345",
        .email = "test@example.com",
        .passwordHash = "hash123",
        .role = "admin",
        .isActive = true,
        .failedLoginAttempts = 0
    };

    EXPECT_EQ(user.id, "usr-12345");
    EXPECT_EQ(user.email, "test@example.com");
    EXPECT_TRUE(user.isActive);
    EXPECT_EQ(user.role, "admin");
    EXPECT_EQ(user.failedLoginAttempts, 0);
}

TEST_CASE("Domain::Entities", "SessionEntityProperties") {
    auto now = std::chrono::system_clock::now();
    Domain::Entities::Session session{
        .id = "sess-abc-123",
        .userId = "usr-12345",
        .jti = "jti-uuid-789",
        .refreshTokenHash = "sha256hash",
        .createdAt = now,
        .lastSeenAt = now,
        .expiresAt = now + std::chrono::hours(24),
        .revokedAt = std::nullopt,
        .revocationReason = std::nullopt,
        .ipAddress = "192.168.1.100",
        .userAgent = "Mozilla/5.0 TestBrowser",
        .deviceName = "Developer MacBook",
        .clientType = "desktop"
    };

    EXPECT_EQ(session.id, "sess-abc-123");
    EXPECT_EQ(session.userId, "usr-12345");
    EXPECT_EQ(session.jti, "jti-uuid-789");
    EXPECT_FALSE(session.revokedAt.has_value());
    EXPECT_EQ(session.clientType, "desktop");
}

TEST_CASE("Domain::Entities", "AuditLogCreation") {
    auto now = std::chrono::system_clock::now();
    Domain::Entities::AuditLog log{
        .id = 1,
        .eventType = "LOGIN_SUCCESS",
        .userId = "usr-123",
        .adminUserId = std::nullopt,
        .sessionId = "sess-123",
        .ipAddress = "127.0.0.1",
        .userAgent = "TestAgent",
        .reason = "Successful authentication",
        .createdAt = now
    };

    EXPECT_EQ(log.eventType, "LOGIN_SUCCESS");
    EXPECT_TRUE(log.userId.has_value());
    EXPECT_EQ(*log.userId, "usr-123");
    EXPECT_FALSE(log.adminUserId.has_value());
}
