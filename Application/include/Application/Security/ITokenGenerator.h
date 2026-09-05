#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace Application::Security {

class ITokenGenerator {
public:
    virtual ~ITokenGenerator() = default;

    virtual std::string generateSecureToken(size_t byteCount = 32) const = 0;
    virtual std::string sha256Hex(std::string_view data) const = 0;
    virtual std::string generateUuid() const = 0;
};

} // namespace Application::Security
