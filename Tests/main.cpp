#include "TestHarness.h"
#include "Infrastructure/Config/EnvLoader.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char* argv[]) {
    // Automatically load .env.test configuration
    Infrastructure::Config::EnvLoader::load(".env.test");

    std::string filter;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg.rfind("--filter=", 0) == 0) {
            filter = std::string(arg.substr(9));
        } else if (arg.rfind("-f=", 0) == 0) {
            filter = std::string(arg.substr(3));
        } else if (i == 1 && arg.front() != '-') {
            filter = std::string(arg);
        }
    }

    const auto& tests = TestHarness::TestRegistry::instance().getTests();

    std::cout << "\n=================================================================\n";
    std::cout << "  CrowApi Comprehensive Multi-Layer Test Suite                    \n";
    std::cout << "  Environment  : .env.test (PostgreSQL 18.6 + OpenSSL 3.x)         \n";
    if (!filter.empty()) {
        std::cout << "  Filter       : " << filter << "\n";
    }
    std::cout << "  Total Tests  : " << tests.size() << "\n";
    std::cout << "=================================================================\n\n";

    size_t passedCount = 0;
    size_t failedCount = 0;
    size_t totalAssertions = 0;
    std::string currentCategory;

    auto suiteStart = std::chrono::high_resolution_clock::now();

    for (const auto& test : tests) {
        if (!filter.empty()) {
            bool matchesCat = test.category.find(filter) != std::string::npos;
            bool matchesName = test.name.find(filter) != std::string::npos;
            if (!matchesCat && !matchesName) {
                continue;
            }
        }

        if (test.category != currentCategory) {
            currentCategory = test.category;
            std::cout << "\n--- [" << currentCategory << "] ---\n";
        }

        auto& ctx = TestHarness::TestContext::instance();
        ctx.resetForTest();

        auto testStart = std::chrono::high_resolution_clock::now();
        try {
            test.func();
        } catch (const std::exception& ex) {
            ctx.recordFailure(__FILE__, __LINE__, std::string("Unhandled exception in test: ") + ex.what());
        } catch (...) {
            ctx.recordFailure(__FILE__, __LINE__, "Unhandled non-std exception in test");
        }
        auto testEnd = std::chrono::high_resolution_clock::now();
        auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(testEnd - testStart).count();

        totalAssertions += ctx.getAssertionsCount();

        if (ctx.hasFailed()) {
            ++failedCount;
            std::cout << "  [FAIL] " << test.name << " (" << durationUs << " us)\n";
            for (const auto& f : ctx.getFailures()) {
                std::cout << "         --> " << f.file << ":" << f.line << " - " << f.message << "\n";
            }
        } else {
            ++passedCount;
            std::cout << "  [PASS] " << test.name << " (" << durationUs << " us)\n";
        }
    }

    auto suiteEnd = std::chrono::high_resolution_clock::now();
    auto totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(suiteEnd - suiteStart).count();

    std::cout << "\n=================================================================\n";
    std::cout << "  TEST EXECUTION SUMMARY\n";
    std::cout << "  Total Executed : " << (passedCount + failedCount) << "\n";
    std::cout << "  Passed         : " << passedCount << "\n";
    std::cout << "  Failed         : " << failedCount << "\n";
    std::cout << "  Assertions     : " << totalAssertions << "\n";
    std::cout << "  Total Time     : " << totalDurationMs << " ms\n";
    std::cout << "=================================================================\n";

    if (failedCount > 0) {
        std::cout << "\n>>> OVERALL RESULT: FAILED (" << failedCount << " test(s) failed) <<<\n\n";
        return 1;
    }

    std::cout << "\n>>> OVERALL RESULT: ALL TESTS PASSED <<<\n\n";
    return 0;
}
