#pragma once

#include "Application/Security/IKeyManager.h"
#include <shared_mutex>
#include <string>
#include <vector>

namespace Infrastructure::Security {

class RsaKeyManager : public Application::Security::IKeyManager {
public:
    explicit RsaKeyManager(
        std::string initialKid = "key-current",
        std::string privateKeyPem = "",
        std::string publicKeyPem = ""
    );

    Application::Security::SigningKey getActiveSigningKey() const override;
    std::optional<std::string> getVerificationPublicKeyPem(std::string_view kid) const override;
    std::string getJwksJson() const override;
    void rotateKey(std::string_view newKid = "") override;

private:
    void rebuildJwksJsonLocked();
    Application::Security::SigningKey createKeyRecord(std::string kid, std::string privPem, std::string pubPem) const;

    mutable std::shared_mutex m_mutex;
    Application::Security::SigningKey m_activeKey;
    std::vector<Application::Security::SigningKey> m_previousKeys;
    std::string m_cachedJwksJson;
};

} // namespace Infrastructure::Security
