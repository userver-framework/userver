#include <utils/periodic_task.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace utils {

PeriodicTask::PeriodicTask()
    : settings_(std::chrono::seconds(1)),
      suspend_state_(SuspendState::kRunning) {}

PeriodicTask::PeriodicTask(std::string name, Settings settings,
                           Callback callback)
    : name_(std::move(name)),
      callback_(std::move(callback)),
      settings_(std::move(settings)),
      suspend_state_(SuspendState::kRunning) {
  DoStart();
}

PeriodicTask::~PeriodicTask() {
  UASSERT(!IsRunning());
  registration_holder_ = std::nullopt;
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

void PeriodicTask::Stop() noexcept {
  if (IsRunning()) {
    LOG_INFO() << "Stopping PeriodicTask with name=" << name_;
    task_.RequestCancel();

    // Do not call `Wait()` here, because it may throw.
    task_ = engine::TaskWithResult<void>();
    LOG_INFO() << "Stopped PeriodicTask with name=" << name_;
  }
}

void PeriodicTask::SetSettings(Settings settings) {
  auto writer = settings_.StartWrite();
  settings.flags = writer->flags;
  *writer = std::move(settings);
  writer.Commit();
}

bool PeriodicTask::SynchronizeDebug(bool preserve_span) {
  if (!IsRunning()) {
    return false;
  }

  return StepDebug(preserve_span);
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
    bool no_exception = Step();
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
  std::lock_guard<engine::Mutex> lock_step(step_mutex_);

  if (suspend_state_.load() == SuspendState::kSuspended) {
    LOG_INFO() << "Skipping suspended PeriodicTask with name=" << name_;
    return true;
  }

  return DoStep();
}

bool PeriodicTask::StepDebug(bool preserve_span) {
  std::lock_guard<engine::Mutex> lock_step(step_mutex_);

  boost::optional<tracing::Span> testsuite_oneshot_span;
  if (preserve_span) {
    tracing::Span span("periodic-synchronize-debug-call");
    testsuite_oneshot_span.emplace(std::move(span));
  }

  return DoStep();
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

void PeriodicTask::SuspendDebug() {
  // step_mutex_ waits, for a potentially long time, for Step() call completion
  std::lock_guard<engine::Mutex> lock_step(step_mutex_);
  auto prior_state = suspend_state_.exchange(SuspendState::kSuspended);
  if (prior_state != SuspendState::kSuspended) {
    LOG_DEBUG() << "Periodic task " << name_ << " suspended";
  }
}

void PeriodicTask::ResumeDebug() {
  auto prior_state = suspend_state_.exchange(SuspendState::kRunning);
  if (prior_state != SuspendState::kRunning) {
    LOG_DEBUG() << "Periodic task " << name_ << " resumed";
  }
}

void PeriodicTask::RegisterInTestsuite(
    testsuite::PeriodicTaskControl& periodic_task_control) {
  registration_holder_.emplace(periodic_task_control, name_, *this);
}

PeriodicTask::Settings PeriodicTask::GetCurrentSettings() const {
  auto settings_ptr = settings_.Read();
  return *settings_ptr;
}

}  // namespace utils
