#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Domain::Common {

enum class Permission {
    SessionReadSelf,
    SessionRevokeSelf,
    SessionReadAll,
    SessionRevokeAll,
    UserManage
};

inline std::string_view permissionToString(Permission perm) {
    switch (perm) {
        case Permission::SessionReadSelf:   return "session:read:self";
        case Permission::SessionRevokeSelf: return "session:revoke:self";
        case Permission::SessionReadAll:    return "session:read:all";
        case Permission::SessionRevokeAll:  return "session:revoke:all";
        case Permission::UserManage:        return "user:manage";
    }
    return "";
}

inline std::vector<Permission> getPermissionsForRole(std::string_view role) {
    if (role == "admin") {
        return {
            Permission::SessionReadSelf,
            Permission::SessionRevokeSelf,
            Permission::SessionReadAll,
            Permission::SessionRevokeAll,
            Permission::UserManage
        };
    }
    // Default normal user permissions
    return {
        Permission::SessionReadSelf,
        Permission::SessionRevokeSelf
    };
}

inline bool roleHasPermission(std::string_view role, Permission targetPerm) {
    const auto perms = getPermissionsForRole(role);
    for (auto p : perms) {
        if (p == targetPerm) return true;
    }
    return false;
}

} // namespace Domain::Common
