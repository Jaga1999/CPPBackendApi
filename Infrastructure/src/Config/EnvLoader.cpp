#include "Infrastructure/Config/EnvLoader.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

namespace Infrastructure::Config {

namespace {

static std::unordered_map<std::string, std::string> s_envMap;
static std::mutex s_envMutex;

std::string getOsEnv(const char* name) {
#if defined(_MSC_VER)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, name) == 0 && buf != nullptr) {
        std::string res(buf);
        free(buf);
        return res;
    }
    return "";
#else
    const char* val = std::getenv(name);
    return val ? std::string(val) : "";
#endif
}

void setOsEnv(const std::string& key, const std::string& val) {
#if defined(_MSC_VER)
    _putenv_s(key.c_str(), val.c_str());
#else
    setenv(key.c_str(), val.c_str(), 1);
#endif
}

std::filesystem::path findEnvFile(const std::string& fileName) {
    namespace fs = std::filesystem;
    fs::path directPath(fileName);
    if (fs::exists(directPath) && !fs::is_directory(directPath)) {
        return directPath;
    }

    // Try current working directory parent traverses up to 4 levels
    fs::path current = fs::current_path();
    for (int i = 0; i < 5; ++i) {
        fs::path candidate = current / fileName;
        if (fs::exists(candidate) && !fs::is_directory(candidate)) {
            return candidate;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    return directPath;
}

} // anonymous namespace

std::string EnvLoader::trim(std::string_view str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return std::string(str.substr(first, (last - first + 1)));
}

std::string EnvLoader::parseValue(std::string_view rawVal) {
    std::string val = trim(rawVal);
    if (val.size() >= 2) {
        if ((val.front() == '"' && val.back() == '"') ||
            (val.front() == '\'' && val.back() == '\'')) {
            return val.substr(1, val.size() - 2);
        }
    }
    // If not quoted, strip inline comment if any
    size_t hashPos = val.find('#');
    if (hashPos != std::string::npos) {
        val = trim(val.substr(0, hashPos));
    }
    return val;
}

bool EnvLoader::load(const std::string& filePath, bool overwriteExisting) {
    std::filesystem::path resolvedPath = findEnvFile(filePath);
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        std::cerr << "[EnvLoader] Warning: Could not open environment file: " << filePath
                  << " (resolved to: " << resolvedPath.string() << ")\n";
        return false;
    }

    std::lock_guard lock(s_envMutex);
    std::string line;
    size_t loadedCount = 0;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }

        size_t eqPos = trimmed.find('=');
        if (eqPos == std::string::npos || eqPos == 0) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, eqPos));
        std::string value = parseValue(trimmed.substr(eqPos + 1));

        if (!key.empty()) {
            bool alreadyInOs = !getOsEnv(key.c_str()).empty();
            if (overwriteExisting || !alreadyInOs || s_envMap.find(key) == s_envMap.end()) {
                s_envMap[key] = value;
                setOsEnv(key, value);
                ++loadedCount;
            }
        }
    }

    std::cout << "[EnvLoader] Loaded " << loadedCount << " variables from: "
              << resolvedPath.string() << "\n";
    return true;
}

std::string EnvLoader::get(const std::string& key, const std::string& defaultValue) {
    std::lock_guard lock(s_envMutex);
    // Check in-memory map first
    auto it = s_envMap.find(key);
    if (it != s_envMap.end()) {
        return it->second;
    }
    // Check OS environment
    std::string osVal = getOsEnv(key.c_str());
    if (!osVal.empty()) {
        return osVal;
    }
    return defaultValue;
}

int EnvLoader::getInt(const std::string& key, int defaultValue) {
    std::string val = get(key, "");
    if (val.empty()) return defaultValue;
    try {
        return std::stoi(val);
    } catch (...) {
        return defaultValue;
    }
}

bool EnvLoader::getBool(const std::string& key, bool defaultValue) {
    std::string val = get(key, "");
    if (val.empty()) return defaultValue;
    std::string lower = val;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
}

void EnvLoader::set(const std::string& key, const std::string& value) {
    std::lock_guard lock(s_envMutex);
    s_envMap[key] = value;
    setOsEnv(key, value);
}

void EnvLoader::clear() {
    std::lock_guard lock(s_envMutex);
    s_envMap.clear();
}

const std::unordered_map<std::string, std::string>& EnvLoader::getAll() {
    std::lock_guard lock(s_envMutex);
    return s_envMap;
}

std::string AppConfig::toDbConnectionString() const {
    std::ostringstream oss;
    oss << "host=" << dbHost
        << " port=" << dbPort
        << " dbname=" << dbName
        << " user=" << dbUser
        << " password=" << dbPassword
        << " connect_timeout=" << dbConnectTimeout;
    return oss.str();
}

AppConfig AppConfig::fromEnv() {
    AppConfig cfg;
    cfg.serverHost = EnvLoader::get("SERVER_HOST", "0.0.0.0");
    cfg.serverPort = static_cast<uint16_t>(EnvLoader::getInt("SERVER_PORT", 8080));
    cfg.logLevel = EnvLoader::get("LOG_LEVEL", "info");
    cfg.serverThreads = EnvLoader::getInt("SERVER_THREADS", 16);

    cfg.dbHost = EnvLoader::get("DB_HOST", "localhost");
    cfg.dbPort = EnvLoader::get("DB_PORT", "5432");
    cfg.dbName = EnvLoader::get("DB_NAME", "crowapi_db");
    cfg.dbUser = EnvLoader::get("DB_USER", "postgres");
    cfg.dbPassword = EnvLoader::get("DB_PASSWORD", "postgres");
    cfg.dbMaxPoolSize = static_cast<size_t>(EnvLoader::getInt("DB_MAX_POOL_SIZE", 20));
    cfg.dbConnectTimeout = EnvLoader::getInt("DB_CONNECT_TIMEOUT", 5);

    cfg.jwtKeyId = EnvLoader::get("JWT_KEY_ID", "key-2026-prod-01");
    cfg.jwtAccessTokenTtlSeconds = EnvLoader::getInt("JWT_ACCESS_TOKEN_TTL_SECONDS", 900);
    cfg.jwtRefreshTokenTtlDays = EnvLoader::getInt("JWT_REFRESH_TOKEN_TTL_DAYS", 30);

    cfg.authMaxFailedAttempts = EnvLoader::getInt("AUTH_MAX_FAILED_ATTEMPTS", 5);
    cfg.authLockoutMinutes = EnvLoader::getInt("AUTH_LOCKOUT_MINUTES", 15);

    cfg.googleClientId = EnvLoader::get("GOOGLE_CLIENT_ID", "");
    cfg.googleClientSecret = EnvLoader::get("GOOGLE_CLIENT_SECRET", "");
    cfg.googleRedirectUri = EnvLoader::get("GOOGLE_REDIRECT_URI", "");

    return cfg;
}

} // namespace Infrastructure::Config
