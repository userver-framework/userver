#include <userver/utest/utest.hpp>

#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(UtilsAsync, Base) {
  auto task = utils::Async("async", [] { return 1; });
  EXPECT_EQ(1, task.Get());
}

UTEST(UtilsAsync, WithDeadlineNotReached) {
  static const auto kVeryLongDuration = std::chrono::seconds(42);
  auto task =
      utils::Async("async", engine::Deadline::FromDuration(kVeryLongDuration),
                   [] { return 1; });
  EXPECT_EQ(task.Get(), 1);
}

UTEST(UtilsAsync, WithDeadlineZero) {
  auto task = utils::Async(
      "async", engine::Deadline::FromDuration(std::chrono::seconds(0)),
      [] { return 1; });
  EXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(UtilsAsync, WithDeadlineCancellationPoint) {
  auto task = utils::Async(
      "async", engine::Deadline::FromDuration(std::chrono::milliseconds(42)),
      [] {
        for (;;) {
          engine::current_task::CancellationPoint();
        }
      });
  EXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(UtilsAsync, MemberFunctions) {
  struct NotCopyable {
    NotCopyable() = default;
    NotCopyable(const NotCopyable&) = delete;
    NotCopyable(NotCopyable&&) = default;
    int MultiplyByTwo(int x) { return x * 2; }
  };

  NotCopyable s;
  auto task =
      utils::Async("async", &NotCopyable::MultiplyByTwo, std::move(s), 2);
  EXPECT_EQ(task.Get(), 4);
}

USERVER_NAMESPACE_END
