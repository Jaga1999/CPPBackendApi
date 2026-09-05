#pragma once

#include <string>
#include <string_view>

namespace Application::Security {

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;

    virtual std::string hashPassword(std::string_view password) const = 0;
    virtual bool verifyPassword(std::string_view password, std::string_view storedHash) const = 0;
};

} // namespace Application::Security
