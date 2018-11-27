#include <utils/periodic_task.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace utils {

PeriodicTask::PeriodicTask(std::string name, Settings settings,
                           Callback callback)
    : name_(std::move(name)), callback_(std::move(callback)) {
  settings_.Set(std::move(settings));
  DoStart();
}

void PeriodicTask::Start(std::string name, Settings settings,
                         Callback callback) {
  Stop();
  name_ = std::move(name);
  settings_.Set(std::move(settings));
  callback_ = std::move(callback);
  DoStart();
}

void PeriodicTask::DoStart() {
  LOG_INFO() << "Starting PeriodicTask with name=" << name_;
  if (settings_.Get()->flags & Flags::kCritical) {
    task_ = engine::CriticalAsync(std::mem_fn(&PeriodicTask::Run), this);
  } else {
    task_ = engine::Async(std::mem_fn(&PeriodicTask::Run), this);
  }
}

void PeriodicTask::Stop() {
  if (task_.IsValid()) {
    LOG_INFO() << "Stopping PeriodicTask with name=" << name_;
    task_.RequestCancel();
    task_.Wait();
    LOG_INFO() << "Stopped PeriodicTask with name=" << name_;
    task_ = engine::TaskWithResult<void>();
  }
}

void PeriodicTask::SetSettings(Settings settings) {
  settings_.Set(std::move(settings));
}

void PeriodicTask::Run() {
  {
    auto settings = settings_.Get();
    if (!(settings->flags & Flags::kNow))
      engine::SleepFor(MutatePeriod(settings->period));
  }

  while (true) {
    const auto before = engine::Deadline::TimePoint::clock::now();

    auto settings = settings_.Get();
    auto period = settings->period;
    const auto exception_period = settings->exception_period.value_or(period);
    const bool strong = static_cast<bool>(settings->flags & Flags::kStrong);
    settings.reset();

    try {
      auto span = tracing::Tracer::GetTracer()->CreateSpanWithoutParent(name_);
      callback_(std::move(span));
    } catch (const std::exception& e) {
      LOG_ERROR() << "Exception in PeriodicTask with name=" << name_ << ": "
                  << e.what();
      period = exception_period;
    }

    if (strong)
      engine::SleepUntil(before + MutatePeriod(period));
    else
      engine::SleepFor(MutatePeriod(period));
  }
}

std::chrono::milliseconds PeriodicTask::MutatePeriod(
    std::chrono::milliseconds period) {
  if (!(settings_.Get()->flags & Flags::kChaotic)) return period;

  const auto distribution = settings_.Get()->distribution;
  const auto ms = std::uniform_int_distribution<int64_t>(
      (period - distribution).count(), (period + distribution).count())(rand_);
  return std::chrono::milliseconds(ms);
}

}  // namespace utils
