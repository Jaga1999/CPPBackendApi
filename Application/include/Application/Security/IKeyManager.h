#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Application::Security {

struct JwkKey {
    std::string kty{"RSA"};
    std::string use{"sig"};
    std::string alg{"RS256"};
    std::string kid;
    std::string n;
    std::string e;
};

struct SigningKey {
    std::string kid;
    std::string privateKeyPem;
    std::string publicKeyPem;
    JwkKey jwk;
};

class IKeyManager {
public:
    virtual ~IKeyManager() = default;

    virtual SigningKey getActiveSigningKey() const = 0;
    virtual std::optional<std::string> getVerificationPublicKeyPem(std::string_view kid) const = 0;
    virtual std::string getJwksJson() const = 0;
    virtual void rotateKey(std::string_view newKid = "") = 0;
};

} // namespace Application::Security
