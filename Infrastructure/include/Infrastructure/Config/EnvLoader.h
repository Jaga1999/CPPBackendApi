#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Infrastructure::Config {

struct AppConfig {
    std::string serverHost{"0.0.0.0"};
    uint16_t serverPort{8080};
    std::string logLevel{"info"};
    int serverThreads{16};

    std::string dbHost{"localhost"};
    std::string dbPort{"5432"};
    std::string dbName{"crowapi_db"};
    std::string dbUser{"postgres"};
    std::string dbPassword{"postgres"};
    size_t dbMaxPoolSize{20};
    int dbConnectTimeout{5};

    std::string jwtKeyId{"key-2026-prod-01"};
    int jwtAccessTokenTtlSeconds{900};
    int jwtRefreshTokenTtlDays{30};

    int authMaxFailedAttempts{5};
    int authLockoutMinutes{15};

    std::string googleClientId{""};
    std::string googleClientSecret{""};
    std::string googleRedirectUri{""};

    [[nodiscard]] std::string toDbConnectionString() const;
    static AppConfig fromEnv();
};

class EnvLoader {
public:
    static bool load(const std::string& filePath = ".env", bool overwriteExisting = false);

    static std::string get(const std::string& key, const std::string& defaultValue = "");
    static int getInt(const std::string& key, int defaultValue = 0);
    static bool getBool(const std::string& key, bool defaultValue = false);

    static void set(const std::string& key, const std::string& value);
    static void clear();

    static const std::unordered_map<std::string, std::string>& getAll();

private:
    static std::string trim(std::string_view str);
    static std::string parseValue(std::string_view rawVal);
};

} // namespace Infrastructure::Config
