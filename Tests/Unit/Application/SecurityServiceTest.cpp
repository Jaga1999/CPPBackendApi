#include "TestHarness.h"
#include "Application/Security/AuthorizationService.h"

TEST_CASE("Application::Security", "AuthorizationServiceRoleCheck") {
    Application::Security::AuthorizationService authService;

    // Admin permissions
    EXPECT_TRUE(authService.hasPermission("admin", Domain::Common::Permission::SessionReadAll));
    EXPECT_TRUE(authService.hasPermission("admin", Domain::Common::Permission::SessionRevokeAll));
    EXPECT_TRUE(authService.hasPermission("admin", Domain::Common::Permission::UserManage));

    // User permissions
    EXPECT_TRUE(authService.hasPermission("user", Domain::Common::Permission::SessionReadSelf));
    EXPECT_TRUE(authService.hasPermission("user", Domain::Common::Permission::SessionRevokeSelf));
    EXPECT_FALSE(authService.hasPermission("user", Domain::Common::Permission::SessionReadAll));
    EXPECT_FALSE(authService.hasPermission("user", Domain::Common::Permission::SessionRevokeAll));
}

TEST_CASE("Application::Security", "AuthorizationServiceSessionAccessAndRevoke") {
    Application::Security::AuthorizationService authService;

    Domain::Entities::User user{.id = "usr-1", .role = "user"};
    Domain::Entities::User admin{.id = "adm-1", .role = "admin"};

    Domain::Entities::Session ownSession{.id = "s-1", .userId = "usr-1"};
    Domain::Entities::Session otherSession{.id = "s-2", .userId = "usr-2"};

    // Resource owner can access and revoke own session
    EXPECT_TRUE(authService.canAccessSession(user, ownSession));
    EXPECT_TRUE(authService.canRevokeSession(user, ownSession));

    // User cannot access or revoke another user's session (IDOR / BOLA defense)
    EXPECT_FALSE(authService.canAccessSession(user, otherSession));
    EXPECT_FALSE(authService.canRevokeSession(user, otherSession));

    // Admin can access and revoke any user's session
    EXPECT_TRUE(authService.canAccessSession(admin, otherSession));
    EXPECT_TRUE(authService.canRevokeSession(admin, otherSession));
}
