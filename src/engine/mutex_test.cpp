#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>

#include <utest/utest.hpp>

TEST(Mutex, WaitAndCancel) {
  RunInCoro([] {
    engine::Mutex mutex;
    std::lock_guard<engine::Mutex> lock(mutex);
    auto task = engine::Async(
        [&mutex]() { std::lock_guard<engine::Mutex> lock(mutex); });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());

    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
  });
}
