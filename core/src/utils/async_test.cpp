#include <utest/utest.hpp>

#include <utils/async.hpp>

TEST(UtilsAsync, Base) {
  RunInCoro([]() {
    auto task = utils::Async("async", [] { return 1; });
    EXPECT_EQ(1, task.Get());
  });
}

TEST(UtilsAsync, WithDeadlineNotReached) {
  RunInCoro([]() {
    static const auto kVeryLongDuration = std::chrono::seconds(42);
    auto task =
        utils::Async("async", engine::Deadline::FromDuration(kVeryLongDuration),
                     [] { return 1; });
    EXPECT_EQ(task.Get(), 1);
  });
}

TEST(UtilsAsync, WithDeadlineZero) {
  RunInCoro([]() {
    auto task = utils::Async(
        "async", engine::Deadline::FromDuration(std::chrono::seconds(0)),
        [] { return 1; });
    EXPECT_THROW(task.Get(), engine::TaskCancelledException);
  });
}

TEST(UtilsAsync, WithDeadlineCancellationPoint) {
  RunInCoro([]() {
    auto task = utils::Async(
        "async", engine::Deadline::FromDuration(std::chrono::milliseconds(42)),
        [] {
          for (;;) {
            engine::current_task::CancellationPoint();
          }
        });
    EXPECT_THROW(task.Get(), engine::TaskCancelledException);
  });
}
