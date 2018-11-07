#include <utest/utest.hpp>

#include <chrono>

#include <engine/async.hpp>
#include <engine/sleep.hpp>

TEST(Task, Ctr) { engine::Task task; }

TEST(Task, Wait) {
  RunInCoro([]() {
    auto task = engine::Async([] {});
    task.Wait();
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Task, WaitFor) {
  RunInCoro([]() {
    auto task = engine::Async([] {});
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Task, Cancel) {
  RunInCoro([]() {
    auto task =
        engine::Async([] { engine::SleepFor(std::chrono::milliseconds(100)); });
    EXPECT_FALSE(task.IsFinished());
    task.RequestCancel();
  });
}

TEST(Task, AutoCancel) {
  RunInCoro([]() {
    auto task =
        engine::Async([] { engine::SleepFor(std::chrono::milliseconds(100)); });
    EXPECT_FALSE(task.IsFinished());
  });
}

TEST(Task, Get) {
  RunInCoro([]() {
    auto result = engine::Async([] { return 12; }).Get();
    EXPECT_EQ(12, result);
  });
}

TEST(Task, GetVoid) {
  RunInCoro([]() {
    engine::Async([] { return; }).Get();
    EXPECT_TRUE(true);
  });
}

TEST(Task, GetException) {
  RunInCoro([]() {
    EXPECT_THROW(engine::Async([] { throw std::runtime_error("123"); }).Get(),
                 std::runtime_error);
  });
}

TEST(Task, GetCancel) {
  RunInCoro([]() {
    auto task =
        engine::Async([] { engine::SleepFor(std::chrono::seconds(10)); });
    task.RequestCancel();
    EXPECT_THROW(task.Get(), std::exception);
  });
}
