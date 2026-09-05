#include "Infrastructure/Security/RsaKeyManager.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include <chrono>
#include <iostream>
#include <sstream>

namespace Infrastructure::Security {

RsaKeyManager::RsaKeyManager(
    std::string initialKid,
    std::string privateKeyPem,
    std::string publicKeyPem
) {
    if (privateKeyPem.empty() || publicKeyPem.empty()) {
        std::cout << "[RsaKeyManager] Generating initial RSA 2048-bit keypair for kid: " << initialKid << "...\n";
        auto [priv, pub] = OpenSslCrypto::generateRsaKeypair(2048);
        privateKeyPem = std::move(priv);
        publicKeyPem = std::move(pub);
    }

    m_activeKey = createKeyRecord(std::move(initialKid), std::move(privateKeyPem), std::move(publicKeyPem));
    rebuildJwksJsonLocked();
}

Application::Security::SigningKey RsaKeyManager::createKeyRecord(
    std::string kid,
    std::string privPem,
    std::string pubPem
) const {
    auto [n, e] = OpenSslCrypto::extractRsaPublicParams(pubPem);
    Application::Security::JwkKey jwk{
        .kty = "RSA",
        .use = "sig",
        .alg = "RS256",
        .kid = kid,
        .n = std::move(n),
        .e = std::move(e)
    };

    return Application::Security::SigningKey{
        .kid = std::move(kid),
        .privateKeyPem = std::move(privPem),
        .publicKeyPem = std::move(pubPem),
        .jwk = std::move(jwk)
    };
}

Application::Security::SigningKey RsaKeyManager::getActiveSigningKey() const {
    std::shared_lock lock(m_mutex);
    return m_activeKey;
}

std::optional<std::string> RsaKeyManager::getVerificationPublicKeyPem(std::string_view kid) const {
    std::shared_lock lock(m_mutex);
    if (m_activeKey.kid == kid) {
        return m_activeKey.publicKeyPem;
    }
    for (const auto& prev : m_previousKeys) {
        if (prev.kid == kid) {
            return prev.publicKeyPem;
        }
    }
    return std::nullopt;
}

std::string RsaKeyManager::getJwksJson() const {
    std::shared_lock lock(m_mutex);
    return m_cachedJwksJson;
}

void RsaKeyManager::rotateKey(std::string_view newKid) {
    std::unique_lock lock(m_mutex);

    std::string kidStr;
    if (newKid.empty()) {
        const auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        kidStr = "key-" + std::to_string(nowSec);
    } else {
        kidStr = std::string(newKid);
    }

    std::cout << "[RsaKeyManager] Rotating active signing key to kid: " << kidStr << "...\n";
    auto [newPriv, newPub] = OpenSslCrypto::generateRsaKeypair(2048);

    // Archive current key to previous keys list for verification of existing tokens
    m_previousKeys.push_back(std::move(m_activeKey));

    // Cap previous keys at 5 to avoid unbounded growth
    if (m_previousKeys.size() > 5) {
        m_previousKeys.erase(m_previousKeys.begin());
    }

    m_activeKey = createKeyRecord(std::move(kidStr), std::move(newPriv), std::move(newPub));
    rebuildJwksJsonLocked();
}

void RsaKeyManager::rebuildJwksJsonLocked() {
    std::ostringstream oss;
    oss << "{\n  \"keys\": [\n";

    auto writeJwk = [&oss](const Application::Security::JwkKey& jwk, bool isLast) {
        oss << "    {\n"
            << "      \"kty\": \"" << jwk.kty << "\",\n"
            << "      \"use\": \"" << jwk.use << "\",\n"
            << "      \"alg\": \"" << jwk.alg << "\",\n"
            << "      \"kid\": \"" << jwk.kid << "\",\n"
            << "      \"n\": \"" << jwk.n << "\",\n"
            << "      \"e\": \"" << jwk.e << "\"\n"
            << "    }" << (isLast ? "\n" : ",\n");
    };

    bool hasPrev = !m_previousKeys.empty();
    writeJwk(m_activeKey.jwk, !hasPrev);

    for (size_t i = 0; i < m_previousKeys.size(); ++i) {
        bool isLast = (i + 1 == m_previousKeys.size());
        writeJwk(m_previousKeys[i].jwk, isLast);
    }

    oss << "  ]\n}";
    m_cachedJwksJson = oss.str();
}

} // namespace Infrastructure::Security
