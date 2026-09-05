#include "TestHarness.h"
#include "Infrastructure/Config/EnvLoader.h"

TEST_CASE("Infrastructure::Config", "EnvLoaderSetAndGet") {
    Infrastructure::Config::EnvLoader::set("TEST_CUSTOM_KEY", "custom_value_123");
    EXPECT_EQ(Infrastructure::Config::EnvLoader::get("TEST_CUSTOM_KEY"), "custom_value_123");
}

TEST_CASE("Infrastructure::Config", "EnvLoaderDefaultFallback") {
    EXPECT_EQ(Infrastructure::Config::EnvLoader::get("NON_EXISTENT_KEY_XYZ", "fallback_val"), "fallback_val");
}

TEST_CASE("Infrastructure::Config", "EnvLoaderTypeConversions") {
    Infrastructure::Config::EnvLoader::set("TEST_INT_VAL", "42");
    EXPECT_EQ(Infrastructure::Config::EnvLoader::getInt("TEST_INT_VAL"), 42);

    Infrastructure::Config::EnvLoader::set("TEST_BOOL_TRUE", "true");
    EXPECT_TRUE(Infrastructure::Config::EnvLoader::getBool("TEST_BOOL_TRUE"));

    Infrastructure::Config::EnvLoader::set("TEST_BOOL_FALSE", "false");
    EXPECT_FALSE(Infrastructure::Config::EnvLoader::getBool("TEST_BOOL_FALSE"));
}

TEST_CASE("Infrastructure::Config", "AppConfigFromEnv") {
    std::string origPort = Infrastructure::Config::EnvLoader::get("SERVER_PORT", "8085");
    std::string origLogLevel = Infrastructure::Config::EnvLoader::get("LOG_LEVEL", "warning");
    std::string origDbName = Infrastructure::Config::EnvLoader::get("DB_NAME", "crowapi_db");

    Infrastructure::Config::EnvLoader::set("SERVER_PORT", "9090");
    Infrastructure::Config::EnvLoader::set("LOG_LEVEL", "debug");
    Infrastructure::Config::EnvLoader::set("DB_NAME", "custom_test_db");

    auto cfg = Infrastructure::Config::AppConfig::fromEnv();
    EXPECT_EQ(cfg.serverPort, 9090);
    EXPECT_EQ(cfg.logLevel, "debug");
    EXPECT_EQ(cfg.dbName, "custom_test_db");
    EXPECT_TRUE(cfg.toDbConnectionString().find("dbname=custom_test_db") != std::string::npos);

    // Restore original test configuration so subsequent DB tests connect to crowapi_db
    Infrastructure::Config::EnvLoader::set("SERVER_PORT", origPort);
    Infrastructure::Config::EnvLoader::set("LOG_LEVEL", origLogLevel);
    Infrastructure::Config::EnvLoader::set("DB_NAME", origDbName);
}
