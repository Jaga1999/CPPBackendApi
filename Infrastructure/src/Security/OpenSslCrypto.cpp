#include "Infrastructure/Security/OpenSslCrypto.h"

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Infrastructure::Security {

namespace {

struct BioDeleter {
    void operator()(BIO* bio) const noexcept {
        if (bio) BIO_free(bio);
    }
};
using BioPtr = std::unique_ptr<BIO, BioDeleter>;

struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* pkey) const noexcept {
        if (pkey) EVP_PKEY_free(pkey);
    }
};
using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;

struct EvpMdCtxDeleter {
    void operator()(EVP_MD_CTX* ctx) const noexcept {
        if (ctx) EVP_MD_CTX_free(ctx);
    }
};
using EvpMdCtxPtr = std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter>;

struct BnDeleter {
    void operator()(BIGNUM* bn) const noexcept {
        if (bn) BN_free(bn);
    }
};
using BnPtr = std::unique_ptr<BIGNUM, BnDeleter>;

std::string bytesToHex(const uint8_t* data, size_t length) {
    std::ostringstream oss;
    for (size_t i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> hexToBytes(std::string_view hex) {
    std::vector<uint8_t> bytes;
    if (hex.length() % 2 != 0) return bytes;
    bytes.reserve(hex.length() / 2);

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString(hex.substr(i, 2));
        auto byte = static_cast<uint8_t>(std::strtoul(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

} // anonymous namespace

std::pair<std::string, std::string> OpenSslCrypto::generateRsaKeypair(int bits) {
    EvpPkeyPtr pkey{EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", static_cast<size_t>(bits))};
    if (!pkey) {
        std::cerr << "[OpenSslCrypto] Failed to generate RSA keypair\n";
        return {"", ""};
    }

    // Write private key PEM
    BioPtr privBio{BIO_new(BIO_s_mem())};
    if (!PEM_write_bio_PKCS8PrivateKey(privBio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr)) {
        return {"", ""};
    }
    char* privData = nullptr;
    long privLen = BIO_get_mem_data(privBio.get(), &privData);
    std::string privPem(privData, privLen);

    // Write public key PEM
    BioPtr pubBio{BIO_new(BIO_s_mem())};
    if (!PEM_write_bio_PUBKEY(pubBio.get(), pkey.get())) {
        return {"", ""};
    }
    char* pubData = nullptr;
    long pubLen = BIO_get_mem_data(pubBio.get(), &pubData);
    std::string pubPem(pubData, pubLen);

    return {std::move(privPem), std::move(pubPem)};
}

std::pair<std::string, std::string> OpenSslCrypto::extractRsaPublicParams(std::string_view publicKeyPem) {
    BioPtr bio{BIO_new_mem_buf(publicKeyPem.data(), static_cast<int>(publicKeyPem.size()))};
    if (!bio) return {"", ""};

    EvpPkeyPtr pkey{PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr)};
    if (!pkey) return {"", ""};

    BIGNUM* nBn = nullptr;
    BIGNUM* eBn = nullptr;

    if (!EVP_PKEY_get_bn_param(pkey.get(), OSSL_PKEY_PARAM_RSA_N, &nBn) ||
        !EVP_PKEY_get_bn_param(pkey.get(), OSSL_PKEY_PARAM_RSA_E, &eBn)) {
        if (nBn) BN_free(nBn);
        if (eBn) BN_free(eBn);
        return {"", ""};
    }

    BnPtr nHolder{nBn};
    BnPtr eHolder{eBn};

    int nLen = BN_num_bytes(nBn);
    std::vector<uint8_t> nBytes(static_cast<size_t>(nLen));
    BN_bn2bin(nBn, nBytes.data());

    int eLen = BN_num_bytes(eBn);
    std::vector<uint8_t> eBytes(static_cast<size_t>(eLen));
    BN_bn2bin(eBn, eBytes.data());

    return {
        base64UrlEncode(nBytes.data(), nBytes.size()),
        base64UrlEncode(eBytes.data(), eBytes.size())
    };
}

std::optional<std::string> OpenSslCrypto::signRs256(std::string_view privateKeyPem, std::string_view message) {
    BioPtr bio{BIO_new_mem_buf(privateKeyPem.data(), static_cast<int>(privateKeyPem.size()))};
    if (!bio) return std::nullopt;

    EvpPkeyPtr pkey{PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr)};
    if (!pkey) return std::nullopt;

    EvpMdCtxPtr ctx{EVP_MD_CTX_new()};
    if (!ctx) return std::nullopt;

    if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) <= 0) {
        return std::nullopt;
    }

    if (EVP_DigestSignUpdate(ctx.get(), message.data(), message.size()) <= 0) {
        return std::nullopt;
    }

    size_t sigLen = 0;
    if (EVP_DigestSignFinal(ctx.get(), nullptr, &sigLen) <= 0 || sigLen == 0) {
        return std::nullopt;
    }

    std::vector<uint8_t> sig(sigLen);
    if (EVP_DigestSignFinal(ctx.get(), sig.data(), &sigLen) <= 0) {
        return std::nullopt;
    }

    return base64UrlEncode(sig.data(), sigLen);
}

bool OpenSslCrypto::verifyRs256(std::string_view publicKeyPem, std::string_view message, std::string_view signatureBase64Url) {
    auto rawSigOpt = base64UrlDecode(signatureBase64Url);
    if (!rawSigOpt.has_value()) {
        return false;
    }

    BioPtr bio{BIO_new_mem_buf(publicKeyPem.data(), static_cast<int>(publicKeyPem.size()))};
    if (!bio) return false;

    EvpPkeyPtr pkey{PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr)};
    if (!pkey) return false;

    EvpMdCtxPtr ctx{EVP_MD_CTX_new()};
    if (!ctx) return false;

    if (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) <= 0) {
        return false;
    }

    if (EVP_DigestVerifyUpdate(ctx.get(), message.data(), message.size()) <= 0) {
        return false;
    }

    const auto& rawSig = *rawSigOpt;
    return EVP_DigestVerifyFinal(ctx.get(), reinterpret_cast<const unsigned char*>(rawSig.data()), rawSig.size()) == 1;
}

std::string OpenSslCrypto::base64UrlEncode(std::string_view data) {
    return base64UrlEncode(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

std::string OpenSslCrypto::base64UrlEncode(const uint8_t* data, size_t length) {
    static const char b64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";

    std::string encoded;
    encoded.reserve(((length + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= length) {
        uint32_t b0 = data[i++];
        uint32_t b1 = data[i++];
        uint32_t b2 = data[i++];

        encoded.push_back(b64Chars[(b0 >> 2) & 0x3F]);
        encoded.push_back(b64Chars[((b0 << 4) | (b1 >> 4)) & 0x3F]);
        encoded.push_back(b64Chars[((b1 << 2) | (b2 >> 6)) & 0x3F]);
        encoded.push_back(b64Chars[b2 & 0x3F]);
    }

    if (i < length) {
        size_t rem = length - i;
        uint32_t b0 = data[i++];
        uint32_t b1 = (rem > 1) ? data[i++] : 0;

        encoded.push_back(b64Chars[(b0 >> 2) & 0x3F]);
        encoded.push_back(b64Chars[((b0 << 4) | (b1 >> 4)) & 0x3F]);
        if (rem == 2) {
            encoded.push_back(b64Chars[(b1 << 2) & 0x3F]);
        }
    }

    return encoded;
}

std::optional<std::string> OpenSslCrypto::base64UrlDecode(std::string_view base64Url) {
    if (base64Url.empty()) return "";

    auto decodeChar = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '-' || c == '+') return 62;
        if (c == '_' || c == '/') return 63;
        if (c == '=') return -2; // padding
        return -1; // invalid
    };

    std::string result;
    result.reserve((base64Url.size() * 3) / 4);

    uint32_t val = 0;
    int valBits = 0;

    for (char c : base64Url) {
        int d = decodeChar(c);
        if (d == -1) return std::nullopt; // invalid character
        if (d == -2) break; // stop at padding '='

        val = (val << 6) | static_cast<uint32_t>(d);
        valBits += 6;

        if (valBits >= 8) {
            valBits -= 8;
            result.push_back(static_cast<char>((val >> valBits) & 0xFF));
        }
    }

    return result;
}

std::string OpenSslCrypto::sha256Hex(std::string_view data) {
    auto raw = sha256Raw(data);
    return bytesToHex(raw.data(), raw.size());
}

std::vector<uint8_t> OpenSslCrypto::sha256Raw(std::string_view data) {
    std::vector<uint8_t> hash(EVP_MAX_MD_SIZE);
    unsigned int hashLen = 0;

    EvpMdCtxPtr ctx{EVP_MD_CTX_new()};
    if (ctx) {
        EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx.get(), data.data(), data.size());
        EVP_DigestFinal_ex(ctx.get(), hash.data(), &hashLen);
    }
    hash.resize(hashLen);
    return hash;
}

std::vector<uint8_t> OpenSslCrypto::randomBytes(size_t length) {
    std::vector<uint8_t> buffer(length);
    if (RAND_bytes(buffer.data(), static_cast<int>(length)) != 1) {
        std::cerr << "[OpenSslCrypto] RAND_bytes failed\n";
    }
    return buffer;
}

std::string OpenSslCrypto::generateSecureToken(size_t byteCount) {
    auto bytes = randomBytes(byteCount);
    return base64UrlEncode(bytes.data(), bytes.size());
}

std::string OpenSslCrypto::generateUuidV4() {
    auto bytes = randomBytes(16);
    // RFC 4122 v4 UUID format:
    // byte 6: version 4 (0100xxxx)
    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0F) | 0x40);
    // byte 8: variant 1 (10xxxxxx)
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3F) | 0x80);

    std::ostringstream oss;
    for (size_t i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            oss << '-';
        }
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

bool OpenSslCrypto::constantTimeCompare(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

std::string OpenSslCrypto::hashPassword(std::string_view password) {
    constexpr int iterations = 100000;
    constexpr size_t saltLen = 32;
    constexpr size_t keyLen = 32;

    auto salt = randomBytes(saltLen);
    std::array<uint8_t, keyLen> derivedKey{};

    PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(keyLen),
        derivedKey.data()
    );

    std::string saltHex = bytesToHex(salt.data(), salt.size());
    std::string keyHex = bytesToHex(derivedKey.data(), derivedKey.size());

    // Cleanse sensitive derived key
    OPENSSL_cleanse(derivedKey.data(), derivedKey.size());

    return "pbkdf2_sha256$" + std::to_string(iterations) + "$" + saltHex + "$" + keyHex;
}

bool OpenSslCrypto::verifyPassword(std::string_view password, std::string_view storedHash) {
    // Format: pbkdf2_sha256$<iterations>$<salt_hex>$<key_hex>
    if (storedHash.rfind("pbkdf2_sha256$", 0) != 0) {
        return false;
    }

    size_t firstDollar = storedHash.find('$', 14);
    if (firstDollar == std::string_view::npos) return false;

    size_t secondDollar = storedHash.find('$', firstDollar + 1);
    if (secondDollar == std::string_view::npos) return false;

    int iterations = std::stoi(std::string(storedHash.substr(14, firstDollar - 14)));
    std::string_view saltHex = storedHash.substr(firstDollar + 1, secondDollar - (firstDollar + 1));
    std::string_view expectedKeyHex = storedHash.substr(secondDollar + 1);

    auto salt = hexToBytes(saltHex);
    auto expectedKey = hexToBytes(expectedKeyHex);
    if (expectedKey.empty() || salt.empty()) return false;

    std::vector<uint8_t> derivedKey(expectedKey.size());
    PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(derivedKey.size()),
        derivedKey.data()
    );

    bool match = CRYPTO_memcmp(derivedKey.data(), expectedKey.data(), expectedKey.size()) == 0;
    OPENSSL_cleanse(derivedKey.data(), derivedKey.size());
    return match;
}

} // namespace Infrastructure::Security
