#include <utils/periodic_task.hpp>

#include <cache/testsuite_support.hpp>
#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace utils {

class PeriodicTask::TestsuiteHolder {
 public:
  TestsuiteHolder(components::TestsuiteSupport& testsuite_support,
                  std::string name, PeriodicTask& task)
      : testsuite_support_(testsuite_support),
        name_(std::move(name)),
        task_(task) {
    testsuite_support_.RegisterPeriodicTask(name_, task_);
  }
  TestsuiteHolder(const TestsuiteHolder&) = delete;
  TestsuiteHolder(TestsuiteHolder&&) = delete;
  TestsuiteHolder& operator=(const TestsuiteHolder&) = delete;
  TestsuiteHolder& operator=(TestsuiteHolder&&) = delete;

  ~TestsuiteHolder() {
    testsuite_support_.UnregisterPeriodicTask(name_, task_);
  }

 private:
  components::TestsuiteSupport& testsuite_support_;
  std::string name_;
  PeriodicTask& task_;
};

PeriodicTask::PeriodicTask()
    : settings_(std::chrono::seconds(1)),
      started_(false),
      callback_succeeded_(false) {}

PeriodicTask::PeriodicTask(std::string name, Settings settings,
                           Callback callback)
    : name_(std::move(name)),
      callback_(std::move(callback)),
      settings_(std::move(settings)),
      started_(false),
      callback_succeeded_(false) {
  DoStart();
}

PeriodicTask::~PeriodicTask() {
  UASSERT(!IsRunning());
  testsuite_holder_.reset();
  Stop();
}

void PeriodicTask::Start(std::string name, Settings settings,
                         Callback callback) {
  Stop();
  name_ = std::move(name);
  settings_.Assign(std::move(settings));
  callback_ = std::move(callback);
  DoStart();
}

void PeriodicTask::DoStart() {
  LOG_INFO() << "Starting PeriodicTask with name=" << name_;
  auto settings_ptr = settings_.Read();
  if (settings_ptr->flags & Flags::kCritical) {
    task_ = engine::impl::CriticalAsync(std::mem_fn(&PeriodicTask::Run), this);
  } else {
    task_ = engine::impl::Async(std::mem_fn(&PeriodicTask::Run), this);
  }
}

bool PeriodicTask::WaitForFirstStep() {
  auto settings_ptr = settings_.Read();
  if (settings_ptr->flags & Flags::kNow) {
    std::unique_lock<engine::Mutex> lock(start_mutex_);
    [[maybe_unused]] auto cv_status =
        start_cv_.Wait(lock, [&] { return started_.load(); });
    return callback_succeeded_;
  }
  return false;
}

void PeriodicTask::Stop() noexcept {
  if (IsRunning()) {
    LOG_INFO() << "Stopping PeriodicTask with name=" << name_;
    task_.RequestCancel();

    // Do not call `Wait()` here, because it may throw.
    task_ = engine::TaskWithResult<void>();
    LOG_INFO() << "Stopped PeriodicTask with name=" << name_;

    started_ = false;
  }
}

void PeriodicTask::SetSettings(Settings settings) {
  auto writer = settings_.StartWrite();
  settings.flags = writer->flags;
  *writer = std::move(settings);
  writer.Commit();
}

bool PeriodicTask::SynchronizeDebug(bool preserve_span) {
  if (!IsRunning()) return false;

  Stop();  // to freely access name_, settings_, callback_

  /* There is a race between current task and a potential task
   * that calls SetSettings(). It should not happen in unittests,
   * other cases are not relevant here.
   */
  auto settings = settings_.ReadCopy();
  settings.flags |= Flags::kNow;

  if (preserve_span) {
    tracing::Span span("periodic-synchronize-debug-call");
    span.DetachFromCoroStack();
    testsuite_oneshot_span_.emplace(std::move(span));
  } else {
    testsuite_oneshot_span_ = boost::none;
  }

  Start(std::move(name_), settings, std::move(callback_));

  return WaitForFirstStep();
}

bool PeriodicTask::IsRunning() const { return task_.IsValid(); }

void PeriodicTask::SleepUntil(engine::Deadline::TimePoint tp) {
  engine::InterruptibleSleepUntil(tp);
}

void PeriodicTask::Run() {
  using clock = engine::Deadline::TimePoint::clock;
  {
    auto settings = settings_.Read();
    if (!(settings->flags & Flags::kNow))
      SleepUntil(clock::now() + MutatePeriod(settings->period));
  }

  while (!engine::current_task::ShouldCancel()) {
    const auto before = clock::now();

    auto no_exception = Step();

    auto settings = settings_.Read();
    auto period = settings->period;
    const auto exception_period = settings->exception_period.value_or(period);
    const bool strong = static_cast<bool>(settings->flags & Flags::kStrong);

    if (!no_exception) period = exception_period;

    SleepUntil((strong ? before : clock::now()) + MutatePeriod(period));
  }
}

bool PeriodicTask::DoStep() {
  auto settings_ptr = settings_.Read();
  const auto span_log_level = settings_ptr->span_level;
  tracing::Span span(name_, tracing::ReferenceType::kChild, span_log_level);
  try {
    callback_();
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception in PeriodicTask with name=" << name_ << ": " << e;
    return false;
  }
}

bool PeriodicTask::Step() {
  boost::optional<tracing::Span> span = std::move(testsuite_oneshot_span_);

  if (span) {
    span->AttachToCoroStack();
    testsuite_oneshot_span_ = boost::none;
  }

  auto result = DoStep();

  if (!started_) {
    std::unique_lock<engine::Mutex> lock(start_mutex_);
    started_ = true;
    callback_succeeded_ = result;
    start_cv_.NotifyAll();
  }

  return result;
}

std::chrono::milliseconds PeriodicTask::MutatePeriod(
    std::chrono::milliseconds period) {
  auto settings_ptr = settings_.Read();
  if (!(settings_ptr->flags & Flags::kChaotic)) return period;

  const auto distribution = settings_ptr->distribution;
  const auto ms = std::uniform_int_distribution<int64_t>(
      (period - distribution).count(), (period + distribution).count())(rand_);
  return std::chrono::milliseconds(ms);
}

void PeriodicTask::RegisterInTestsuite(
    components::TestsuiteSupport& testsuite_support) {
  testsuite_holder_ =
      std::make_unique<TestsuiteHolder>(testsuite_support, name_, *this);
}

}  // namespace utils
