#include <userver/utest/utest.hpp>

#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(UtilsAsync, Base) {
  auto task = utils::Async("async", [] { return 1; });
  EXPECT_EQ(1, task.Get());
}

UTEST(UtilsAsync, WithDeadlineNotReached) {
  auto task = utils::Async(
      "async", engine::Deadline::FromDuration(utest::kMaxTestWaitTime),
      [] { return 1; });
  EXPECT_EQ(task.Get(), 1);
}

UTEST(UtilsAsync, WithPassedDeadline) {
  auto task = utils::Async(
      "async", engine::Deadline::FromDuration(std::chrono::seconds(-1)),
      [] { return 1; });
  UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(UtilsAsync, WithDeadlineCancellationPoint) {
  auto task = utils::Async(
      "async", engine::Deadline::FromDuration(std::chrono::milliseconds(42)),
      [] {
        for (;;) {
          engine::current_task::CancellationPoint();
        }
      });
  UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
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
