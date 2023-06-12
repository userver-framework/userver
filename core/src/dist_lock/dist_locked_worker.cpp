#include <userver/dist_lock/dist_locked_worker.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <dist_lock/impl/helpers.hpp>
#include <dist_lock/impl/locker.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {

DistLockedWorker::DistLockedWorker(
    std::string name, WorkerFunc worker_func,
    std::shared_ptr<DistLockStrategyBase> strategy,
    const DistLockSettings& settings, engine::TaskProcessor* task_processor)
    : locker_ptr_(std::make_shared<impl::Locker>(std::move(name),
                                                 std::move(strategy), settings,
                                                 std::move(worker_func))),
      task_processor_(task_processor) {}

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

engine::TaskProcessor& DistLockedWorker::GetTaskProcessor() const noexcept {
  return task_processor_ ? *task_processor_
                         : engine::current_task::GetTaskProcessor();
}

void DistLockedWorker::UpdateSettings(const DistLockSettings& settings) {
  locker_ptr_->SetSettings(settings);
}

void DistLockedWorker::Start() {
  LOG_INFO() << "Starting DistLockedWorker " << Name();

  std::lock_guard<engine::Mutex> lock(locker_task_mutex_);
  locker_task_ =
      locker_ptr_->RunAsync(GetTaskProcessor(), impl::LockerMode::kWorker,
                            DistLockWaitingMode::kWait);

  LOG_INFO() << "Started DistLockedWorker " << Name();
}

void DistLockedWorker::RunOnce() {
  LOG_INFO() << "Running DistLockedWorker once " << Name();

  std::lock_guard<engine::Mutex> lock(locker_task_mutex_);
  locker_task_ =
      locker_ptr_->RunAsync(GetTaskProcessor(), impl::LockerMode::kOneshot,
                            DistLockWaitingMode::kWait);
  locker_task_.Get();

  LOG_INFO() << "Running DistLockedWorker once done" << Name();
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

void DumpMetric(utils::statistics::Writer& writer,
                const DistLockedWorker& worker) {
  const auto& stats = worker.GetStatistics();
  const bool running = worker.IsRunning();
  const auto locked_duration = worker.GetLockedDuration();

  writer["running"] = running ? 1 : 0;
  writer["locked"] = locked_duration ? 1 : 0;
  writer["locked-for-ms"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          locked_duration.value_or(std::chrono::seconds(0)))
          .count();

  writer["successes"] = stats.lock_successes.Load();
  writer["failures"] = stats.lock_failures.Load();
  writer["watchdog-triggers"] = stats.watchdog_triggers.Load();
  writer["brain-splits"] = stats.brain_splits.Load();
  writer["task-failures"] = stats.task_failures.Load();
}

}  // namespace dist_lock

USERVER_NAMESPACE_END
