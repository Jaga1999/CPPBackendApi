#pragma once

#include "Domain/Common/Permissions.h"
#include "Domain/Entities/Session.h"
#include "Domain/Entities/User.h"
#include <string_view>

namespace Application::Security {

class AuthorizationService {
public:
    [[nodiscard]] bool hasPermission(std::string_view role, Domain::Common::Permission perm) const noexcept;
    [[nodiscard]] bool hasPermission(const Domain::Entities::User& user, Domain::Common::Permission perm) const noexcept;

    [[nodiscard]] bool canAccessSession(
        const Domain::Entities::User& user,
        const Domain::Entities::Session& session
    ) const noexcept;

    [[nodiscard]] bool canRevokeSession(
        const Domain::Entities::User& user,
        const Domain::Entities::Session& session
    ) const noexcept;
};

} // namespace Application::Security
