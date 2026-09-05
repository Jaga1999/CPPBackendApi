#pragma once

#include "Application/Security/IPasswordHasher.h"
#include "Application/Security/ITokenGenerator.h"
#include "Infrastructure/Security/OpenSslCrypto.h"

namespace Infrastructure::Security {

class OpenSslPasswordHasher : public Application::Security::IPasswordHasher {
public:
    std::string hashPassword(std::string_view password) const override {
        return OpenSslCrypto::hashPassword(password);
    }

    bool verifyPassword(std::string_view password, std::string_view storedHash) const override {
        return OpenSslCrypto::verifyPassword(password, storedHash);
    }
};

class OpenSslTokenGenerator : public Application::Security::ITokenGenerator {
public:
    std::string generateSecureToken(size_t byteCount = 32) const override {
        return OpenSslCrypto::generateSecureToken(byteCount);
    }

    std::string sha256Hex(std::string_view data) const override {
        return OpenSslCrypto::sha256Hex(data);
    }

    std::string generateUuid() const override {
        return OpenSslCrypto::generateUuidV4();
    }
};

} // namespace Infrastructure::Security
