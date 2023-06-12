#include <userver/utils/periodic_task.hpp>

#include <random>

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/rand.hpp>

#include <compiler/tls.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

USERVER_PREVENT_TLS_CACHING
std::minstd_rand& GetFastRandomBitsGenerator() {
  thread_local std::minstd_rand rand{utils::Rand()};
  return rand;
}

}  // namespace

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
  registration_holder_ = std::nullopt;
  Stop();
}

void PeriodicTask::Start(std::string name, Settings settings,
                         Callback callback) {
  UASSERT_MSG(!name.empty(), "Periodic task must have a name");
  auto name_ptr = name_.StartWrite();
  UASSERT_MSG(name_ptr->empty() || name == *name_ptr,
              fmt::format("PeriodicTask name must not be changed "
                          "on the fly, old={}, new={}",
                          *name_ptr, name));
  Stop();
  *name_ptr = std::move(name);
  name_ptr.Commit();
  settings_.Assign(std::move(settings));
  callback_ = std::move(callback);
  DoStart();
}

void PeriodicTask::DoStart() {
  const auto name_ptr = name_.Read();
  LOG_INFO() << "Starting PeriodicTask with name=" << *name_ptr;
  auto settings_ptr = settings_.Read();
  auto& task_processor = settings_ptr->task_processor
                             ? *settings_ptr->task_processor
                             : engine::current_task::GetTaskProcessor();
  if (settings_ptr->flags & Flags::kCritical) {
    task_ =
        engine::CriticalAsyncNoSpan(task_processor, &PeriodicTask::Run, this);
  } else {
    task_ = engine::AsyncNoSpan(task_processor, &PeriodicTask::Run, this);
  }
}

void PeriodicTask::Stop() noexcept {
  const auto name_ptr = name_.Read();
  try {
    if (IsRunning()) {
      LOG_INFO() << "Stopping PeriodicTask with name=" << *name_ptr;
      task_.SyncCancel();
      changed_event_.Reset();
      should_force_step_ = false;
      task_ = engine::TaskWithResult<void>();
      LOG_INFO() << "Stopped PeriodicTask with name=" << *name_ptr;
    }
  } catch (std::exception& e) {
    LOG_ERROR() << "Exception while stopping PeriodicTask with name="
                << *name_ptr << ": " << e;
  } catch (...) {
    LOG_ERROR() << "Exception while stopping PeriodicTask with name="
                << *name_ptr;
  }
}

void PeriodicTask::SetSettings(Settings settings) {
  bool has_changed{};
  {
    auto writer = settings_.StartWrite();
    settings.flags = writer->flags;
    has_changed = settings.period != writer->period;
    *writer = std::move(settings);
    writer.Commit();
  }

  if (has_changed) {
    const auto name_ptr = name_.Read();
    LOG_DEBUG() << "periodic task settings have changed, signalling name="
                << *name_ptr;
    changed_event_.Send();
  }
}

void PeriodicTask::ForceStepAsync() {
  should_force_step_ = true;
  changed_event_.Send();
}

bool PeriodicTask::SynchronizeDebug(bool preserve_span) {
  if (!IsRunning()) {
    return false;
  }

  return StepDebug(preserve_span);
}

bool PeriodicTask::IsRunning() const { return task_.IsValid(); }

void PeriodicTask::Run() {
  bool skip_step = false;
  {
    auto settings = settings_.Read();
    if (!(settings->flags & Flags::kNow)) {
      skip_step = true;
    }
  }

  while (!engine::current_task::ShouldCancel()) {
    const auto before = std::chrono::steady_clock::now();
    bool no_exception = true;

    if (!std::exchange(skip_step, false)) {
      no_exception = Step();
    }

    const auto settings = settings_.Read();
    auto period = settings->period;
    const auto exception_period = settings->exception_period.value_or(period);

    if (!no_exception) period = exception_period;

    std::chrono::steady_clock::time_point start;
    if (settings->flags & Flags::kStrong) {
      start = before;
    } else {
      start = std::chrono::steady_clock::now();
    }

    while (changed_event_.WaitForEventUntil(start + MutatePeriod(period))) {
      if (should_force_step_.exchange(false)) {
        break;
      }
      // The config variable value has been changed, reload
      const auto settings = settings_.Read();
      period = settings->period;
      const auto exception_period = settings->exception_period.value_or(period);
      if (!no_exception) period = exception_period;
    }
  }
}

bool PeriodicTask::DoStep() {
  auto settings_ptr = settings_.Read();
  const auto span_log_level = settings_ptr->span_level;
  const auto name_ptr = name_.Read();
  tracing::Span span(*name_ptr, tracing::ReferenceType::kChild, span_log_level);
  try {
    callback_();
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception in PeriodicTask with name=" << *name_ptr << ": "
                << e;
    return false;
  }
}

bool PeriodicTask::Step() {
  std::lock_guard<engine::Mutex> lock_step(step_mutex_);

  if (suspend_state_.load() == SuspendState::kSuspended) {
    const auto name_ptr = name_.Read();
    LOG_INFO() << "Skipping suspended PeriodicTask with name=" << *name_ptr;
    return true;
  }

  return DoStep();
}

bool PeriodicTask::StepDebug(bool preserve_span) {
  std::lock_guard<engine::Mutex> lock_step(step_mutex_);

  std::optional<tracing::Span> testsuite_oneshot_span;
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
      (period - distribution).count(),
      (period + distribution).count())(GetFastRandomBitsGenerator());
  return std::chrono::milliseconds(ms);
}

void PeriodicTask::SuspendDebug() {
  // step_mutex_ waits, for a potentially long time, for Step() call completion
  std::lock_guard<engine::Mutex> lock_step(step_mutex_);
  auto prior_state = suspend_state_.exchange(SuspendState::kSuspended);
  if (prior_state != SuspendState::kSuspended) {
    const auto name_ptr = name_.Read();
    LOG_DEBUG() << "Periodic task " << *name_ptr << " suspended";
  }
}

void PeriodicTask::ResumeDebug() {
  auto prior_state = suspend_state_.exchange(SuspendState::kRunning);
  if (prior_state != SuspendState::kRunning) {
    const auto name_ptr = name_.Read();
    LOG_DEBUG() << "Periodic task " << *name_ptr << " resumed";
  }
}

void PeriodicTask::RegisterInTestsuite(
    testsuite::PeriodicTaskControl& periodic_task_control) {
  const auto name_ptr = name_.Read();
  registration_holder_.emplace(periodic_task_control, *name_ptr, *this);
}

PeriodicTask::Settings PeriodicTask::GetCurrentSettings() const {
  auto settings_ptr = settings_.Read();
  return *settings_ptr;
}

}  // namespace utils

USERVER_NAMESPACE_END
