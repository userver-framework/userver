#include <utils/periodic_task.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace utils {

PeriodicTask::PeriodicTask()
    : settings_(std::chrono::seconds(1)), started_(false) {}

PeriodicTask::PeriodicTask(std::string name, Settings settings,
                           Callback callback)
    : name_(std::move(name)),
      callback_(std::move(callback)),
      settings_(std::move(settings)),
      started_(false) {
  DoStart();
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
  if (settings_.Read()->flags & Flags::kCritical) {
    task_ = engine::impl::CriticalAsync(std::mem_fn(&PeriodicTask::Run), this);
  } else {
    task_ = engine::impl::Async(std::mem_fn(&PeriodicTask::Run), this);
  }
}

void PeriodicTask::WaitForFirstStep() {
  if (settings_.Read()->flags & Flags::kNow) {
    std::unique_lock<engine::Mutex> lock(start_mutex_);
    [[maybe_unused]] auto cv_status =
        start_cv_.Wait(lock, [&] { return started_.load(); });
  }
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
  settings_.Assign(std::move(settings));
}

void PeriodicTask::SynchronizeDebug() {
  if (!IsRunning()) return;

  Stop();  // to freely access name_, settings_, callback_

  /* There is a race between current task and a potential task
   * that calls SetSettings(). It should not happen in unittests,
   * other cases are not relevant here.
   */
  auto settings = *settings_.Read();
  settings.flags |= Flags::kNow;

  Start(std::move(name_), settings, std::move(callback_));

  WaitForFirstStep();
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
  const auto span_log_level = settings_.Read()->span_level;
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
  auto result = DoStep();

  if (!started_) {
    std::unique_lock<engine::Mutex> lock(start_mutex_);
    started_ = true;
    start_cv_.NotifyAll();
  }

  return result;
}

std::chrono::milliseconds PeriodicTask::MutatePeriod(
    std::chrono::milliseconds period) {
  if (!(settings_.Read()->flags & Flags::kChaotic)) return period;

  const auto distribution = settings_.Read()->distribution;
  const auto ms = std::uniform_int_distribution<int64_t>(
      (period - distribution).count(), (period + distribution).count())(rand_);
  return std::chrono::milliseconds(ms);
}

}  // namespace utils
