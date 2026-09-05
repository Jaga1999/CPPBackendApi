#include "TestHarness.h"
#include "Presentation/Common/HttpResponseHelper.h"
#include <crow/json.h>

TEST_CASE("Presentation::HttpResponse", "SuccessResponseEnvelope") {
    auto apiResp = Application::Common::ApiResponse<void>::ok("Operation succeeded");
    auto resp = Presentation::Common::HttpResponseHelper::success(apiResp);
    EXPECT_EQ(resp.code, 200);

    auto parsed = crow::json::load(resp.body);
    EXPECT_TRUE(parsed);
    EXPECT_TRUE(parsed["success"].b());
    EXPECT_EQ(parsed["statusCode"].i(), 200);
    EXPECT_EQ(parsed["message"].s(), "Operation succeeded");
}

TEST_CASE("Presentation::HttpResponse", "SerializeTodoDto") {
    Application::DTOs::TodoResponse dto{
        .id = 123,
        .title = "Sample Todo",
        .description = "Test Desc",
        .completed = true
    };
    auto wval = Presentation::Common::HttpResponseHelper::serializeTodoDto(dto);
    auto parsed = crow::json::load(wval.dump());
    EXPECT_TRUE(parsed);
    EXPECT_EQ(parsed["id"].i(), 123);
    EXPECT_EQ(parsed["title"].s(), "Sample Todo");
    EXPECT_TRUE(parsed["completed"].b());
}

TEST_CASE("Presentation::HttpResponse", "ErrorResponseEnvelope") {
    auto resp = Presentation::Common::HttpResponseHelper::error(400, "Validation failed", {"Field 'title' is required"});
    EXPECT_EQ(resp.code, 400);

    auto parsed = crow::json::load(resp.body);
    EXPECT_TRUE(parsed);
    EXPECT_FALSE(parsed["success"].b());
    EXPECT_EQ(parsed["statusCode"].i(), 400);
    EXPECT_EQ(parsed["message"].s(), "Validation failed");
    EXPECT_TRUE(parsed["errors"].size() == 1);
    EXPECT_EQ(parsed["errors"][0].s(), "Field 'title' is required");
}
