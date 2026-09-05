#include "TestHarness.h"
#include "Application/Validation/InputValidator.h"
#include "Application/Validation/AuthInputValidator.h"

TEST_CASE("Application::Validation", "TodoInputValidatorSuccess") {
    Application::DTOs::CreateTodoRequest req{
        .title = "Valid Title",
        .description = "Optional description"
    };
    auto res = Application::Validation::InputValidator::validateCreate(req);
    EXPECT_TRUE(res.isSuccess());
}

TEST_CASE("Application::Validation", "TodoInputValidatorEmptyTitle") {
    Application::DTOs::CreateTodoRequest req{
        .title = "",
        .description = "description"
    };
    auto res = Application::Validation::InputValidator::validateCreate(req);
    EXPECT_FALSE(res.isSuccess());
    EXPECT_TRUE(!res.error().details.empty());
}

TEST_CASE("Application::Validation", "TodoInputValidatorWhitespaceTitle") {
    Application::DTOs::CreateTodoRequest req{
        .title = "    ",
        .description = "description"
    };
    auto res = Application::Validation::InputValidator::validateCreate(req);
    EXPECT_FALSE(res.isSuccess());
    EXPECT_TRUE(!res.error().details.empty());
}

TEST_CASE("Application::Validation", "AuthInputValidatorEmailSuccess") {
    Application::DTOs::RegisterRequest req{
        .email = "alice@example.com",
        .password = "SecurePassword123!"
    };
    auto res = Application::Validation::AuthInputValidator::validateRegister(req);
    EXPECT_TRUE(res.isSuccess());
}

TEST_CASE("Application::Validation", "AuthInputValidatorInvalidEmail") {
    Application::DTOs::RegisterRequest req1{
        .email = "invalid-email",
        .password = "SecurePassword123!"
    };
    auto res1 = Application::Validation::AuthInputValidator::validateRegister(req1);
    EXPECT_FALSE(res1.isSuccess());

    Application::DTOs::RegisterRequest req2{
        .email = "@missinguser.com",
        .password = "SecurePassword123!"
    };
    auto res2 = Application::Validation::AuthInputValidator::validateRegister(req2);
    EXPECT_FALSE(res2.isSuccess());

    Application::DTOs::RegisterRequest req3{
        .email = "alice@",
        .password = "SecurePassword123!"
    };
    auto res3 = Application::Validation::AuthInputValidator::validateRegister(req3);
    EXPECT_FALSE(res3.isSuccess());
}

TEST_CASE("Application::Validation", "AuthInputValidatorShortPassword") {
    Application::DTOs::RegisterRequest req{
        .email = "alice@example.com",
        .password = "short"
    };
    auto res = Application::Validation::AuthInputValidator::validateRegister(req);
    EXPECT_FALSE(res.isSuccess());
    EXPECT_TRUE(!res.error().details.empty());
}

TEST_CASE("Application::Validation", "AuthInputValidatorEmptyPassword") {
    Application::DTOs::RegisterRequest req{
        .email = "alice@example.com",
        .password = ""
    };
    auto res = Application::Validation::AuthInputValidator::validateRegister(req);
    EXPECT_FALSE(res.isSuccess());
}
