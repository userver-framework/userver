#include <userver/utils/async.hpp>

#include <vector>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/numeric.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>

#include <engine/ev/thread_control.hpp>

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
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    int MultiplyByTwo(int x) { return x * 2; }
  };

  NotCopyable s;
  auto task =
      utils::Async("async", &NotCopyable::MultiplyByTwo, std::move(s), 2);
  EXPECT_EQ(task.Get(), 4);
}

// Test from https://github.com/userver-framework/userver/issues/48 by
// https://github.com/itrofimow
UTEST(UtilsAsync, FromNonWorkerThread) {
  auto& ev_thread = engine::current_task::GetEventThread();
  auto& task_processor = engine::current_task::GetTaskProcessor();
  engine::TaskWithResult<void> task;

  ev_thread.RunInEvLoopSync([&task_processor, &task] {
    task = utils::Async(task_processor, "just_a_task", [] {});
  });

  task.Wait();
}

namespace {

using Request = int;
using Response = int;

/// [AsyncBackground component]
class AsyncRequestProcessor final {
 public:
  AsyncRequestProcessor();

  void FooAsync(Request&& request);

  Response WaitAndGetAggregate();

 private:
  static Response Foo(Request&& request);

  engine::TaskProcessor& task_processor_;
  concurrent::Variable<std::vector<engine::TaskWithResult<Response>>> tasks_;
};
/// [AsyncBackground component]

UTEST(UtilsAsync, AsyncBackground) {
  AsyncRequestProcessor async_request_processor;

  /// [AsyncBackground handler]
  auto handler = [&](Request&& request) {
    async_request_processor.FooAsync(std::move(request));
    return "Please wait, your request is being processed.";
  };
  /// [AsyncBackground handler]

  UEXPECT_NO_THROW(handler(2));
  UEXPECT_NO_THROW(handler(3));
  EXPECT_EQ(async_request_processor.WaitAndGetAggregate(), 10);
}

AsyncRequestProcessor::AsyncRequestProcessor()
    : task_processor_(engine::current_task::GetTaskProcessor()) {}

/// [AsyncBackground FooAsync]
void AsyncRequestProcessor::FooAsync(Request&& request) {
  auto tasks = tasks_.Lock();
  tasks->push_back(
      utils::AsyncBackground("foo", task_processor_, &Foo, std::move(request)));
}
/// [AsyncBackground FooAsync]

Response AsyncRequestProcessor::WaitAndGetAggregate() {
  using boost::adaptors::transformed;
  auto tasks = tasks_.Lock();
  auto get_result = transformed([](auto& task) { return task.Get(); });
  return boost::accumulate(*tasks | get_result, 0);
}

Response AsyncRequestProcessor::Foo(Request&& request) { return request * 2; }

}  // namespace

USERVER_NAMESPACE_END
