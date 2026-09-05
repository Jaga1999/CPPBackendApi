#pragma once

#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace TestHarness {

struct TestFailure {
    std::string file;
    int line;
    std::string message;
};

class TestContext {
public:
    static TestContext& instance() {
        static TestContext s_ctx;
        return s_ctx;
    }

    void recordPass() {
        ++m_currentAssertions;
    }

    void recordFailure(std::string file, int line, std::string message) {
        m_currentFailures.push_back({std::move(file), line, std::move(message)});
    }

    bool hasFailed() const {
        return !m_currentFailures.empty();
    }

    const std::vector<TestFailure>& getFailures() const {
        return m_currentFailures;
    }

    void resetForTest() {
        m_currentAssertions = 0;
        m_currentFailures.clear();
    }

    size_t getAssertionsCount() const {
        return m_currentAssertions;
    }

private:
    size_t m_currentAssertions{0};
    std::vector<TestFailure> m_currentFailures;
};

struct TestCaseInfo {
    std::string category;
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry s_reg;
        return s_reg;
    }

    void registerTest(std::string category, std::string name, std::function<void()> func) {
        m_tests.push_back({std::move(category), std::move(name), std::move(func)});
    }

    const std::vector<TestCaseInfo>& getTests() const {
        return m_tests;
    }

private:
    std::vector<TestCaseInfo> m_tests;
};

struct AutoTestRegister {
    AutoTestRegister(std::string category, std::string name, std::function<void()> func) {
        TestRegistry::instance().registerTest(std::move(category), std::move(name), std::move(func));
    }
};

} // namespace TestHarness

#define TEST_CASE_STR_JOIN(a, b) a##b
#define TEST_CASE_JOIN(a, b) TEST_CASE_STR_JOIN(a, b)

#define TEST_CASE(category_str, name_str) \
    static void TEST_CASE_JOIN(test_func_, __LINE__)(); \
    static ::TestHarness::AutoTestRegister TEST_CASE_JOIN(test_reg_, __LINE__)( \
        category_str, name_str, &TEST_CASE_JOIN(test_func_, __LINE__)); \
    static void TEST_CASE_JOIN(test_func_, __LINE__)()

#define EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            ::TestHarness::TestContext::instance().recordFailure(__FILE__, __LINE__, "EXPECT_TRUE failed: (" #expr ")"); \
        } else { \
            ::TestHarness::TestContext::instance().recordPass(); \
        } \
    } while (0)

#define EXPECT_FALSE(expr) \
    do { \
        if (expr) { \
            ::TestHarness::TestContext::instance().recordFailure(__FILE__, __LINE__, "EXPECT_FALSE failed: (" #expr ")"); \
        } else { \
            ::TestHarness::TestContext::instance().recordPass(); \
        } \
    } while (0)

#define EXPECT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::ostringstream oss; \
            oss << "EXPECT_EQ failed: (" #a " == " #b ") [" << (a) << " != " << (b) << "]"; \
            ::TestHarness::TestContext::instance().recordFailure(__FILE__, __LINE__, oss.str()); \
        } else { \
            ::TestHarness::TestContext::instance().recordPass(); \
        } \
    } while (0)

#define EXPECT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::ostringstream oss; \
            oss << "EXPECT_NE failed: (" #a " != " #b ") [" << (a) << " == " << (b) << "]"; \
            ::TestHarness::TestContext::instance().recordFailure(__FILE__, __LINE__, oss.str()); \
        } else { \
            ::TestHarness::TestContext::instance().recordPass(); \
        } \
    } while (0)

#define EXPECT_THROW(expr, exception_type) \
    do { \
        bool caught = false; \
        try { \
            expr; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) { \
            caught = false; \
        } \
        if (!caught) { \
            ::TestHarness::TestContext::instance().recordFailure(__FILE__, __LINE__, "EXPECT_THROW failed: expected " #exception_type " for (" #expr ")"); \
        } else { \
            ::TestHarness::TestContext::instance().recordPass(); \
        } \
    } while (0)

#define EXPECT_NO_THROW(expr) \
    do { \
        bool threw = false; \
        std::string exMsg; \
        try { \
            expr; \
        } catch (const std::exception& ex) { \
            threw = true; \
            exMsg = ex.what(); \
        } catch (...) { \
            threw = true; \
            exMsg = "unknown non-std exception"; \
        } \
        if (threw) { \
            ::TestHarness::TestContext::instance().recordFailure(__FILE__, __LINE__, "EXPECT_NO_THROW failed: (" #expr ") threw " + exMsg); \
        } else { \
            ::TestHarness::TestContext::instance().recordPass(); \
        } \
    } while (0)
