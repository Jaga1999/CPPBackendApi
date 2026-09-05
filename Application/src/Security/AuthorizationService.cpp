#include "Application/Security/AuthorizationService.h"

namespace Application::Security {

bool AuthorizationService::hasPermission(std::string_view role, Domain::Common::Permission perm) const noexcept {
    return Domain::Common::roleHasPermission(role, perm);
}

bool AuthorizationService::hasPermission(const Domain::Entities::User& user, Domain::Common::Permission perm) const noexcept {
    return hasPermission(user.role, perm);
}

bool AuthorizationService::canAccessSession(
    const Domain::Entities::User& user,
    const Domain::Entities::Session& session
) const noexcept {
    if (hasPermission(user, Domain::Common::Permission::SessionReadAll)) {
        return true;
    }
    return (user.id == session.userId) && hasPermission(user, Domain::Common::Permission::SessionReadSelf);
}

bool AuthorizationService::canRevokeSession(
    const Domain::Entities::User& user,
    const Domain::Entities::Session& session
) const noexcept {
    if (hasPermission(user, Domain::Common::Permission::SessionRevokeAll)) {
        return true;
    }
    return (user.id == session.userId) && hasPermission(user, Domain::Common::Permission::SessionRevokeSelf);
}

} // namespace Application::Security
