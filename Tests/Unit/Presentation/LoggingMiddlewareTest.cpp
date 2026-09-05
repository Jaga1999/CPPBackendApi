#include "TestHarness.h"
#include "Presentation/Middleware/LoggingMiddleware.h"

TEST_CASE("Presentation::Logging", "LogLevelParsing") {
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("verbose") == Presentation::Middleware::AppLogLevel::Verbose);
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("debug") == Presentation::Middleware::AppLogLevel::Debug);
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("info") == Presentation::Middleware::AppLogLevel::Info);
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("warning") == Presentation::Middleware::AppLogLevel::Warning);
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("error") == Presentation::Middleware::AppLogLevel::Error);
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("critical") == Presentation::Middleware::AppLogLevel::Critical);

    // Case insensitive
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("DEBUG") == Presentation::Middleware::AppLogLevel::Debug);
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("InFo") == Presentation::Middleware::AppLogLevel::Info);

    // Fallback on invalid
    EXPECT_TRUE(Presentation::Middleware::parseLogLevel("unknown_val") == Presentation::Middleware::AppLogLevel::Info);
}
