#include <userver/utest/utest.hpp>

#include <condition_variable>
#include <mutex>

#include <boost/algorithm/string/split.hpp>

#include <logging/logging_test.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
/* Scheduler is dumb, but the life is short. */
const auto kSlowRatio = 10;
}  // namespace

UTEST(PeriodicTask, Noop) { utils::PeriodicTask task; }

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
  utils::PeriodicTask task("task", std::chrono::milliseconds(100), []() {});

  EXPECT_TRUE(task.IsRunning());
  // ~PeriodicTask() should call Stop()
}

UTEST(PeriodicTask, StartWithSeconds) {
  // Ensure that the expression compiles without curly brackets around seconds
  utils::PeriodicTask task("task", std::chrono::seconds(100), []() {});

  EXPECT_TRUE(task.IsRunning());
  // ~PeriodicTask() should call Stop()
}

namespace {
struct SimpleTaskData {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  long c = 0;
  long sleep_ms = 0;
  bool throw_exception = false;

  auto GetTaskFunction() {
    return [this] { Run(); };
  }

  void Run() {
    engine::SleepFor(std::chrono::milliseconds(sleep_ms));
    std::unique_lock<engine::Mutex> lock(mutex);
    c++;
    LOG_DEBUG() << "SimpleTaskData::Run";
    cv.NotifyOne();

    if (throw_exception) throw std::runtime_error("error_msg");
  }

  int GetCount() const { return c; }

  template <typename Duration, typename Pred>
  auto WaitFor(Duration duration, Pred pred) {
    std::unique_lock<engine::Mutex> lock(mutex);
    return cv.WaitFor(lock, duration, pred);
  }
};
}  // namespace

UTEST(PeriodicTask, SingleRun) {
  SimpleTaskData simple;

  auto period = std::chrono::milliseconds(10);

  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(period * kSlowRatio,
                             [&simple]() { return simple.GetCount() > 0; }));
  task.Stop();
}

UTEST(PeriodicTask, MultipleRun) {
  SimpleTaskData simple;

  auto period = std::chrono::milliseconds(3);
  auto n = 10;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple, n]() { return simple.GetCount() > n; }));
  task.Stop();
}

UTEST(PeriodicTask, Period) {
  SimpleTaskData simple;

  auto period = std::chrono::milliseconds(3);
  auto n = 10;

  auto start = std::chrono::steady_clock::now();
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple, n]() { return simple.GetCount() > n; }));
  auto finish = std::chrono::steady_clock::now();

  EXPECT_GE(finish - start, period * n);

  task.Stop();
}

UTEST(PeriodicTask, Slow) {
  SimpleTaskData simple;

  simple.sleep_ms = 10;
  auto period = std::chrono::milliseconds(2);
  auto full_period = period + std::chrono::milliseconds(simple.sleep_ms);
  auto n = 5;

  auto start = std::chrono::steady_clock::now();
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(full_period * n * kSlowRatio,
                             [&simple, n]() { return simple.GetCount() > n; }));
  auto finish = std::chrono::steady_clock::now();

  EXPECT_GE(finish - start, full_period * n);

  task.Stop();
}

UTEST(PeriodicTask, Strong) {
  SimpleTaskData simple;

  simple.sleep_ms = 30;
  auto period = std::chrono::milliseconds(10);
  auto n = 7;

  auto start = std::chrono::steady_clock::now();
  utils::PeriodicTask task("task",
                           utils::PeriodicTask::Settings(
                               period, utils::PeriodicTask::Flags::kStrong),
                           simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(
      std::chrono::milliseconds(simple.sleep_ms) * n * kSlowRatio,
      [&simple, n]() { return simple.GetCount() > n; }));
  auto finish = std::chrono::steady_clock::now();

  EXPECT_GE(finish - start, std::chrono::milliseconds(simple.sleep_ms) * n);

  task.Stop();
}

UTEST(PeriodicTask, Now) {
  SimpleTaskData simple;

  auto timeout = std::chrono::milliseconds(50);

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

  auto timeout = std::chrono::milliseconds(50);

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

  auto period = std::chrono::milliseconds(50);
  utils::PeriodicTask::Settings settings(period,
                                         utils::PeriodicTask::Flags::kNow);
  settings.exception_period = std::chrono::milliseconds(10);

  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(*settings.exception_period * kSlowRatio,
                             [&simple]() { return simple.GetCount() > 0; }));

  task.Stop();
}

UTEST(PeriodicTask, SetSettings) {
  SimpleTaskData simple;

  auto period1 = std::chrono::milliseconds(50);
  utils::PeriodicTask::Settings settings(period1);

  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  auto period2 = std::chrono::milliseconds(10);
  settings.period = period2;
  task.SetSettings(settings);

  auto n = 4;
  EXPECT_TRUE(simple.WaitFor(period1 + period2 * n * kSlowRatio,
                             [&simple, n]() { return simple.GetCount() > n; }));

  task.Stop();
}

UTEST(PeriodicTask, SetSettingsInstant) {
  SimpleTaskData simple;
  auto period1 = std::chrono::seconds(120);
  utils::PeriodicTask::Settings settings(period1,
                                         utils::PeriodicTask::Flags::kNow);
  utils::PeriodicTask task("task", settings, simple.GetTaskFunction());

  auto kSyncDuration = std::chrono::seconds(1);
  engine::SleepFor(kSyncDuration);

  auto period2 = kSyncDuration / 20;
  settings.period = period2;
  task.SetSettings(settings);

  engine::SleepFor(kSyncDuration);

  EXPECT_TRUE(simple.WaitFor(2 * period2,
                             [&simple]() { return simple.GetCount() > 0; }));
  task.Stop();
}

UTEST(PeriodicTask, StopStop) {
  SimpleTaskData simple;

  auto period = std::chrono::milliseconds(5);
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(simple.WaitFor(utest::kMaxTestWaitTime,
                             [&simple]() { return simple.GetCount() > 0; }));
  task.Stop();
  task.Stop();
}

UTEST(PeriodicTask, StopDefault) {
  SimpleTaskData simple;

  utils::PeriodicTask task;
  task.Stop();
}

UTEST(PeriodicTask, Restart) {
  SimpleTaskData simple;

  auto period = std::chrono::milliseconds(10);
  auto n = 5;
  utils::PeriodicTask task;
  EXPECT_FALSE(task.IsRunning());
  task.Start("task", period, simple.GetTaskFunction());
  EXPECT_TRUE(task.IsRunning());

  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple, n]() { return simple.GetCount() > n; }));
  task.Stop();
  EXPECT_FALSE(task.IsRunning());
}

UTEST(PeriodicTask, SynchronizeDebug) {
  SimpleTaskData simple;

  auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());

  EXPECT_FALSE(simple.WaitFor(std::chrono::milliseconds(10),
                              [&simple]() { return simple.GetCount() > 0; }));

  bool status = task.SynchronizeDebug();
  EXPECT_TRUE(status);
  EXPECT_GE(simple.GetCount(), 1);

  task.Stop();
}

UTEST(PeriodicTask, SynchronizeDebugFailure) {
  SimpleTaskData simple;

  simple.throw_exception = true;

  auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask task("task", period, simple.GetTaskFunction());

  EXPECT_FALSE(simple.WaitFor(std::chrono::milliseconds(10),
                              [&simple]() { return simple.GetCount() > 0; }));

  bool status = task.SynchronizeDebug();
  EXPECT_FALSE(status);
  EXPECT_GE(simple.GetCount(), 1);

  task.Stop();
}

UTEST(PeriodicTask, SynchronizeDebugSpan) {
  tracing::Span span(__func__);
  SimpleTaskData simple;
  std::string task_link;

  auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask task("task", period, [&task_link] {
    task_link = tracing::Span::CurrentSpan().GetLink();
  });

  bool status = task.SynchronizeDebug(true);
  EXPECT_TRUE(status);
  EXPECT_EQ(task_link, span.GetLink());
  task.Stop();
}

class PeriodicTaskLog : public LoggingTest {};

UTEST_F(PeriodicTaskLog, ErrorLog) {
  SimpleTaskData simple;
  simple.throw_exception = true;

  auto period = utest::kMaxTestWaitTime;
  utils::PeriodicTask::Settings settings(period,
                                         utils::PeriodicTask::Flags::kNow);
  auto n = 5;
  utils::PeriodicTask task;
  task.Start("task_name", settings, simple.GetTaskFunction());

  EXPECT_TRUE(simple.WaitFor(period * n * kSlowRatio,
                             [&simple]() { return simple.GetCount() > 0; }));

  std::vector<std::string> log_strings;
  auto log = GetStreamString();
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
