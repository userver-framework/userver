#include <dist_lock/dist_locked_worker.hpp>

#include <engine/async.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/statistics/metadata.hpp>

#include <dist_lock/impl/helpers.hpp>
#include <dist_lock/impl/locker.hpp>

namespace dist_lock {

DistLockedWorker::DistLockedWorker(
    std::string name, WorkerFunc worker_func,
    std::shared_ptr<DistLockStrategyBase> strategy,
    const DistLockSettings& settings)
    : locker_ptr_(std::make_shared<impl::Locker>(std::move(name),
                                                 std::move(strategy), settings,
                                                 std::move(worker_func))) {}

DistLockedWorker::~DistLockedWorker() {
  UASSERT_MSG(!IsRunning(), "Stop() was not called");
  UASSERT(!locker_task_.IsValid());
}

const std::string& DistLockedWorker::Name() const {
  return locker_ptr_->Name();
}

DistLockSettings DistLockedWorker::GetSettings() const {
  return locker_ptr_->GetSettings();
}

void DistLockedWorker::UpdateSettings(const DistLockSettings& settings) {
  locker_ptr_->SetSettings(settings);
}

void DistLockedWorker::Start() {
  LOG_INFO() << "Starting DistLockedWorker " << Name();

  std::lock_guard<engine::Mutex> lock(locker_task_mutex_);
  // Locker creates its own Span in Run()
  locker_task_ = engine::impl::CriticalAsync([this] {
    locker_ptr_->Run(impl::LockerMode::kWorker, DistLockWaitingMode::kWait);
  });

  LOG_INFO() << "Started DistLockedWorker " << Name();
}

void DistLockedWorker::Stop() {
  LOG_INFO() << "Stopping DistLockedWorker " << Name();

  std::lock_guard<engine::Mutex> lock(locker_task_mutex_);
  if (locker_task_.IsValid()) locker_task_.RequestCancel();
  impl::GetTask(locker_task_, impl::LockerName(Name()));

  LOG_INFO() << "Stopped DistLocked Worker " << Name();
}

bool DistLockedWorker::IsRunning() const {
  std::lock_guard<engine::Mutex> lock(locker_task_mutex_);
  return locker_task_.IsValid();
}

std::optional<std::chrono::steady_clock::duration>
DistLockedWorker::GetLockedDuration() const {
  return locker_ptr_->GetLockedDuration();
}

const Statistics& DistLockedWorker::GetStatistics() const {
  return locker_ptr_->GetStatistics();
}

formats::json::Value DistLockedWorker::GetStatisticsJson() const {
  const auto& stats = GetStatistics();
  bool running = IsRunning();
  auto locked_duration = GetLockedDuration();

  formats::json::ValueBuilder result;
  result["running"] = running ? 1 : 0;
  result["locked"] = locked_duration ? 1 : 0;
  result["locked-for-ms"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          locked_duration.value_or(std::chrono::seconds(0)))
          .count();

  result["successes"] = stats.successes.Load();
  result["failures"] = stats.failures.Load();
  result["watchdog-triggers"] = stats.watchdog_triggers.Load();
  result["brain-splits"] = stats.brain_splits.Load();
  ::utils::statistics::SolomonLabelValue(result, "distlock_name");

  return result.ExtractValue();
}

}  // namespace dist_lock
