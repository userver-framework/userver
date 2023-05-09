#include <userver/utest/utest.hpp>

#include <boost/algorithm/string/split.hpp>

#include <logging/logging_test.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/periodic_task.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {
/* Scheduler is dumb, but the life is short. */
const auto kSlowRatio = 10;
}  // namespace

UTEST(PeriodicTask, Noop) { const utils::PeriodicTask task; }

UTEST(PeriodicTask, StopWithoutStart) {
  utils::PeriodicTask task;
  task.Stop();
}

UTEST(PeriodicTask, StartStop) {
  utils::PeriodicTask task("task", std::chrono::milliseconds(100), []() {});

  EXPECT_TRUE(task.IsRunning());
  task.Stop();
  EXPECT_FALSE(task.IsRunning());
}

UTEST(PeriodicTask, StartNoStop) {
  const utils::PeriodicTask task("task", std::chrono::milliseconds(100),
                                 []() {});

  EXPECT_TRUE(task.IsRunning());
  // ~PeriodicTask() should call Stop()
}

UTEST(PeriodicTask, StartWithSeconds) {
  // Ensure that the expression compiles without curly brackets around seconds
  const utils::PeriodicTask task("task", std::chrono::seconds(100), []() {});

  EXPECT_TRUE(task.IsRunning());
  // ~PeriodicTask() should call Stop()
}

namespace {

using Count = std::size_t;

struct SimpleTaskData final {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  Count count = 0;
  std::chrono::milliseconds sleep{0};
  bool throw_exception = false;

  auto GetTaskFunction() {
    return [this] { Run(); };
  }

  void Run() {
    engine::SleepFor(sleep);
    const std::unique_lock lock(mutex);
    ++count;
    LOG_DEBUG() << "SimpleTaskData::Run";
    cv.NotifyOne();

    if (throw_exception) throw std::runtime_error("error_msg");
  }

  Count GetCount() const { return count; }

  template <typename Duration, typename Pred>
  auto WaitFor(Duration duration, Pred pred) {
    std::unique_lock<engine::Mutex> lock(mutex);
    return cv.WaitFor(lock, duration, pred);
  }
};

}  // namespace

UTEST(PeriodicTask, SingleRun) {
  SimpleTaskData simple;

  const auto period = 10ms;

  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(period * kSlowRatio,
                             [&simple]() { return simple.GetCount() > 0; }));
  task.Stop();
}

UTEST(PeriodicTask, MultipleRun) {
  SimpleTaskData simple;

  constexpr auto period = 3ms;
  constexpr Count n = 10;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple]() { return simple.GetCount() > n; }));
  task.Stop();
}

UTEST(PeriodicTask, Period) {
  SimpleTaskData simple;

  constexpr auto period = 3ms;
  constexpr Count n = 10;

  const auto start = std::chrono::steady_clock::now();
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple]() { return simple.GetCount() > n; }));
  const auto finish = std::chrono::steady_clock::now();

  EXPECT_GE(finish - start, period * n);

  task.Stop();
}

UTEST(PeriodicTask, Slow) {
  SimpleTaskData simple;

  simple.sleep = 10ms;
  constexpr auto period = 2ms;
  const auto full_period = period + simple.sleep;
  constexpr Count n = 5;

  const auto start = std::chrono::steady_clock::now();
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(full_period * n * kSlowRatio,
                             [&] { return simple.GetCount() > n; }));
  const auto finish = std::chrono::steady_clock::now();

  EXPECT_GE(finish - start, full_period * n);

  task.Stop();
}

UTEST(PeriodicTask, Strong) {
  SimpleTaskData simple;

  simple.sleep = 30ms;
  constexpr auto period = 10ms;
  constexpr Count n = 7;

  const auto start = std::chrono::steady_clock::now();
  utils::PeriodicTask task("task",
                           utils::PeriodicTask::Settings(
                               period, utils::PeriodicTask::Flags::kStrong),
                           simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(simple.sleep * n * kSlowRatio,
                             [&] { return simple.GetCount() > n; }));
  const auto finish = std::chrono::steady_clock::now();

  EXPECT_GE(finish - start, simple.sleep * n);

  task.Stop();
}

UTEST(PeriodicTask, Now) {
  SimpleTaskData simple;

  constexpr auto timeout = 50ms;

  utils::PeriodicTask task(
      "task",
      utils::PeriodicTask::Settings(utest::kMaxTestWaitTime,
                                    utils::PeriodicTask::Flags::kNow),
      simple.GetTaskFunction());
  EXPECT_TRUE(
      simple.WaitFor(timeout, [&simple]() { return simple.GetCount() > 0; }));

  task.Stop();
}

UTEST(PeriodicTask, NotNow) {
  SimpleTaskData simple;

  constexpr auto timeout = 50ms;

  utils::PeriodicTask task(
      "task", utils::PeriodicTask::Settings{utest::kMaxTestWaitTime},
      simple.GetTaskFunction());
  EXPECT_FALSE(
      simple.WaitFor(timeout, [&simple]() { return simple.GetCount() > 0; }));

  task.Stop();
}

UTEST(PeriodicTask, ExceptionPeriod) {
  SimpleTaskData simple;
  simple.throw_exception = true;

  constexpr auto period = 50ms;
  utils::PeriodicTask::Settings settings(period,
                                         utils::PeriodicTask::Flags::kNow);
  settings.exception_period = 10ms;

  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(*settings.exception_period * kSlowRatio,
                             [&simple]() { return simple.GetCount() > 0; }));

  task.Stop();
}

UTEST(PeriodicTask, SetSettings) {
  SimpleTaskData simple;

  constexpr auto period1 = 50ms;
  utils::PeriodicTask::Settings settings(period1);

  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  constexpr auto period2 = 10ms;
  settings.period = period2;
  task.SetSettings(settings);

  constexpr Count n = 4;
  EXPECT_TRUE(simple.WaitFor(period1 + period2 * n * kSlowRatio,
                             [&simple]() { return simple.GetCount() > n; }));

  task.Stop();
}

UTEST(PeriodicTask, SetSettingsInstant) {
  SimpleTaskData simple;
  constexpr auto period1 = 120s;
  utils::PeriodicTask::Settings settings(period1,
                                         utils::PeriodicTask::Flags::kNow);
  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  constexpr auto kSyncDuration = 1s;
  engine::SleepFor(kSyncDuration);

  constexpr auto period2 = kSyncDuration / 20;
  settings.period = period2;
  task.SetSettings(settings);

  engine::SleepFor(kSyncDuration);

  EXPECT_TRUE(simple.WaitFor(2 * period2,
                             [&simple]() { return simple.GetCount() > 0; }));
  task.Stop();
}

UTEST(PeriodicTask, SetSettingsFirstIteration) {
  SimpleTaskData simple;

  constexpr auto period1 = 50ms;

  utils::PeriodicTask task("task", period1, simple.GetTaskFunction());

  constexpr auto period2 = 120s;
  task.SetSettings(period2);

  EXPECT_TRUE(simple.WaitFor(period1 * kSlowRatio,
                             [&simple]() { return simple.GetCount() == 0; }));

  task.Stop();
}

UTEST(PeriodicTask, ForceStepAsync) {
  SimpleTaskData simple;

  constexpr auto period = 100ms;
  const utils::PeriodicTask::Settings settings(
      period, utils::PeriodicTask::Flags::kNow);
  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  simple.WaitFor(period * kSlowRatio,
                 [&simple]() { return simple.GetCount() > 0; });

  constexpr Count n = 30;
  constexpr auto kSyncDuration = 10ms;
  for (Count i = 0; i != n; i++) {
    engine::SleepFor(kSyncDuration);
    task.ForceStepAsync();
  }

  EXPECT_TRUE(simple.WaitFor(
      period * kSlowRatio, [&simple]() { return simple.GetCount() >= n + 2; }));
}

UTEST(PeriodicTask, ForceStepAsyncInstant) {
  SimpleTaskData simple;

  constexpr auto period = 120s;
  const utils::PeriodicTask::Settings settings(
      period, utils::PeriodicTask::Flags::kNow);
  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  constexpr auto kSyncDuration = 100ms;
  engine::SleepFor(kSyncDuration);

  task.ForceStepAsync();

  EXPECT_TRUE(
      simple.WaitFor(100ms, [&simple]() { return simple.GetCount() == 1; }));
  task.Stop();
}

UTEST(PeriodicTask, ForceStepAsyncFirstIteration) {
  SimpleTaskData simple;

  constexpr auto period = 120s;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());

  task.ForceStepAsync();

  EXPECT_TRUE(
      simple.WaitFor(100ms, [&simple]() { return simple.GetCount() == 1; }));
  task.Stop();
}

UTEST(PeriodicTask, ForceStepAsyncPeriod) {
  SimpleTaskData simple;

  constexpr auto period = 50ms;
  const utils::PeriodicTask::Settings settings(
      period, utils::PeriodicTask::Flags::kNow);
  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  constexpr Count n = 60;
  constexpr auto kSyncDuration = 10ms;
  for (Count i = 0; i != n; i++) {
    engine::SleepFor(kSyncDuration);
    task.ForceStepAsync();
  }

  EXPECT_FALSE(simple.WaitFor(period * kSlowRatio, [&simple]() {
    return simple.GetCount() >= n + 11;
  }));
  task.Stop();
}

UTEST(PeriodicTask, StopStop) {
  SimpleTaskData simple;

  constexpr auto period = 5ms;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(utest::kMaxTestWaitTime,
                             [&simple]() { return simple.GetCount() > 0; }));
  task.Stop();
  task.Stop();
}

UTEST(PeriodicTask, StopDefault) {
  utils::PeriodicTask task;
  task.Stop();
}

UTEST(PeriodicTask, Restart) {
  SimpleTaskData simple;

  constexpr auto period = 10ms;
  constexpr Count n = 5;
  utils::PeriodicTask task;
  EXPECT_FALSE(task.IsRunning());
  task.Start("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(task.IsRunning());

  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple]() { return simple.GetCount() > n; }));
  task.Stop();
  EXPECT_FALSE(task.IsRunning());
}

UTEST(PeriodicTask, SynchronizeDebug) {
  SimpleTaskData simple;

  constexpr auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());

  EXPECT_FALSE(
      simple.WaitFor(10ms, [&simple]() { return simple.GetCount() > 0; }));

  const bool status = task.SynchronizeDebug();
  EXPECT_TRUE(status);
  EXPECT_GE(simple.GetCount(), 1);

  task.Stop();
}

UTEST(PeriodicTask, SynchronizeDebugFailure) {
  SimpleTaskData simple;

  simple.throw_exception = true;

  constexpr auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());

  EXPECT_FALSE(
      simple.WaitFor(10ms, [&simple]() { return simple.GetCount() > 0; }));

  const bool status = task.SynchronizeDebug();
  EXPECT_FALSE(status);
  EXPECT_GE(simple.GetCount(), 1);

  task.Stop();
}

UTEST(PeriodicTask, SynchronizeDebugSpan) {
  const tracing::Span span(__func__);
  std::string task_link;

  constexpr auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask task("task", period, [&task_link] {
    task_link = tracing::Span::CurrentSpan().GetLink();
  });

  const bool status = task.SynchronizeDebug(true);
  EXPECT_TRUE(status);
  EXPECT_EQ(task_link, span.GetLink());
  task.Stop();
}

class PeriodicTaskLog : public LoggingTest {};

UTEST_F(PeriodicTaskLog, ErrorLog) {
  SimpleTaskData simple;
  simple.throw_exception = true;

  constexpr auto period = utest::kMaxTestWaitTime;
  const utils::PeriodicTask::Settings settings(
      period, utils::PeriodicTask::Flags::kNow);
  constexpr Count n = 5;
  utils::PeriodicTask task;
  task.Start("task_name", settings, simple.GetTaskFunction());

  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple]() { return simple.GetCount() > 0; }));

  std::vector<std::string> log_strings;
  const auto log = GetStreamString();
  int error_msg = 0;
  boost::split(log_strings, log, [](char c) { return c == '\n'; });
  for (const auto& str : log_strings) {
    if (str.find("error_msg") != std::string::npos) {
      error_msg++;
      EXPECT_NE(std::string::npos, str.find("span_id="));
    }
  }
  EXPECT_EQ(1, error_msg);

  task.Stop();
}

USERVER_NAMESPACE_END
