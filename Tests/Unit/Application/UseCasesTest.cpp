#include "TestHarness.h"
#include "Application/UseCases/TodoUseCases.h"
#include "Infrastructure/Persistence/InMemoryTodoRepository.h"

TEST_CASE("Application::UseCases", "TodoUseCasesCreateAndRetrieve") {
    auto repo = std::make_shared<Infrastructure::Persistence::InMemoryTodoRepository>();
    Application::UseCases::CreateTodoUseCase createUseCase(repo);
    Application::UseCases::GetTodoByIdUseCase getUseCase(repo);
    Application::UseCases::ListTodosUseCase listUseCase(repo);

    Application::DTOs::CreateTodoRequest req{
        .title = "Learn Clean Architecture",
        .description = "Test Description"
    };

    auto created = createUseCase.execute(req);
    EXPECT_TRUE(created.isSuccess());
    EXPECT_TRUE(created.value().data.has_value());
    EXPECT_TRUE(created.value().data->id > 0);
    EXPECT_EQ(created.value().data->title, "Learn Clean Architecture");

    uint64_t id = created.value().data->id;
    auto fetched = getUseCase.execute(id);
    EXPECT_TRUE(fetched.isSuccess());
    EXPECT_TRUE(fetched.value().data.has_value());
    EXPECT_EQ(fetched.value().data->id, id);
    EXPECT_EQ(fetched.value().data->title, "Learn Clean Architecture");

    auto list = listUseCase.execute();
    EXPECT_TRUE(list.data.has_value());
    EXPECT_TRUE(list.data->size() >= 1);
}

TEST_CASE("Application::UseCases", "TodoUseCasesUpdate") {
    auto repo = std::make_shared<Infrastructure::Persistence::InMemoryTodoRepository>();
    Application::UseCases::CreateTodoUseCase createUseCase(repo);
    Application::UseCases::UpdateTodoUseCase updateUseCase(repo);

    auto created = createUseCase.execute({.title = "Original Title", .description = "Original Desc"});
    EXPECT_TRUE(created.isSuccess());
    uint64_t id = created.value().data->id;

    Application::DTOs::UpdateTodoRequest updateReq{
        .title = "Updated Title",
        .description = "Updated Desc",
        .completed = true
    };

    auto updated = updateUseCase.execute(id, updateReq);
    EXPECT_TRUE(updated.isSuccess());
    EXPECT_TRUE(updated.value().data.has_value());
    EXPECT_EQ(updated.value().data->title, "Updated Title");
    EXPECT_TRUE(updated.value().data->completed);
}

TEST_CASE("Application::UseCases", "TodoUseCasesDelete") {
    auto repo = std::make_shared<Infrastructure::Persistence::InMemoryTodoRepository>();
    Application::UseCases::CreateTodoUseCase createUseCase(repo);
    Application::UseCases::DeleteTodoUseCase deleteUseCase(repo);
    Application::UseCases::GetTodoByIdUseCase getUseCase(repo);

    auto created = createUseCase.execute({.title = "To be deleted", .description = ""});
    EXPECT_TRUE(created.isSuccess());
    uint64_t id = created.value().data->id;

    auto deletedRes = deleteUseCase.execute(id);
    EXPECT_TRUE(deletedRes.isSuccess());

    auto fetched = getUseCase.execute(id);
    EXPECT_FALSE(fetched.isSuccess());
}
