#include <userver/ydb/dist_lock/worker.hpp>

#include <userver/engine/future.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/ydb/exceptions.hpp>

#include <ydb/impl/dist_lock/semaphore_settings.hpp>
#include <ydb/impl/dist_lock/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

bool IsStopped(CoordinationSession& session) {
  return NYdb::NCoordination::EConnectionState::STOPPED ==
         session.GetConnectionState();
}

class BinarySemaphore final {
 public:
  BinarySemaphore(CoordinationSession& session, std::string_view semaphore_name,
                  impl::dist_lock::Statistics& stats)
      : session_(session), semaphore_name_(semaphore_name), stats_(stats) {}

  [[nodiscard]] bool Acquire() {
    try {
      LOG_DEBUG() << "Acquire semaphore " << semaphore_name_;
      const auto result = session_.AcquireSemaphore(
          semaphore_name_,
          NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(
              impl::dist_lock::kSemaphoreLimit));
      UINVARIANT(result, "unexpected AcquireSemaphore result");
      ++stats_.lock_successes;
      return true;
    } catch (const ydb::OperationCancelledError& /*ex*/) {
      return false;
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Acquire semaphore error: " << ex;
      ++stats_.lock_failures;
      return false;
    }
  }

  void Release() noexcept {
    try {
      LOG_DEBUG() << "Release semaphore " << semaphore_name_;
      const engine::TaskCancellationBlocker cancel_blocker;
      session_.ReleaseSemaphore(semaphore_name_);
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Release semaphore error: " << ex;
    }
  }

 private:
  CoordinationSession& session_;
  std::string_view semaphore_name_;
  impl::dist_lock::Statistics& stats_;
};

class SessionTask final {
 public:
  SessionTask(CoordinationSession&& session,
              engine::Future<void>&& session_stopped,
              std::string_view semaphore_name,
              const DistLockedWorker::Callback& callback,
              const DistLockSettings& settings,
              impl::dist_lock::Statistics& stats, std::atomic<bool>& owns_lock)
      : session_{std::move(session)},
        session_stopped_{std::move(session_stopped)},
        semaphore_{session_, semaphore_name, stats},
        callback_{callback},
        settings_{settings},
        stats_{stats},
        owns_lock_{owns_lock} {}

  void Run(bool run_once) {
    while (!engine::current_task::ShouldCancel() && !IsStopped(session_)) {
      if (!semaphore_.Acquire()) {
        engine::InterruptibleSleepFor(settings_.acquire_interval);
        continue;
      }

      LOG_INFO() << "Semaphore acquired";
      owns_lock_.store(true);

      LOG_INFO() << "Executing task";
      auto task = utils::CriticalAsync("dist-locked-task", callback_);

      const auto idx = engine::WaitAny(session_stopped_, task);

      // If session stopped, then we have lost (or are going to lose) the lock.
      // If task stopped, then we are going to release the lock.
      // If we are cancelled, then we are going to release or drop the lock.
      owns_lock_.store(false);

      if (!idx.has_value()) {
        UASSERT(engine::current_task::ShouldCancel());
        CancelTask(std::move(task));
        semaphore_.Release();
        return;
      }

      const auto from_time = utils::datetime::SteadyNow();

      if (0 == *idx) {
        LOG_INFO() << "Session stopped";
        ++stats_.session_stopped;
        CancelTask(std::move(task));
        engine::InterruptibleSleepUntil(from_time +
                                        settings_.restart_session_delay);
        return;
      }

      UASSERT_MSG(1 == *idx, "unexpected idx value: " + std::to_string(*idx));
      LOG_INFO() << "Task finished";
      semaphore_.Release();
      CheckTaskSucceeded(std::move(task));
      if (run_once) {
        return;
      }
      engine::InterruptibleSleepUntil(from_time + settings_.restart_delay);
    }
  }

 private:
  void CancelTask(engine::TaskWithResult<void>&& task) {
    task.RequestCancel();
    const engine::TaskCancellationBlocker cancel_blocker;
    task.WaitFor(settings_.cancel_task_time_limit);
    if (!task.IsFinished()) {
      LOG_WARNING() << "Cancel task time limit exceeded";
      ++stats_.cancel_task_time_limit_exceeded;
      task.Wait();
    }
    CheckTaskSucceeded(std::move(task));
  }

  void CheckTaskSucceeded(engine::TaskWithResult<void>&& task) {
    try {
      task.Get();
      ++stats_.task_successes;
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Task error: " << ex;
      ++stats_.task_failures;
    }
  }

  CoordinationSession session_;
  engine::Future<void> session_stopped_;
  BinarySemaphore semaphore_;
  const DistLockedWorker::Callback& callback_;
  const DistLockSettings& settings_;
  impl::dist_lock::Statistics& stats_;
  std::atomic<bool>& owns_lock_;
};

}  // namespace

DistLockedWorker::DistLockedWorker(
    engine::TaskProcessor& task_processor,
    std::shared_ptr<CoordinationClient> coordination_client,
    std::string_view coordination_node, std::string_view semaphore_name,
    DistLockSettings settings, Callback callback)
    : task_processor_{task_processor},
      coordination_client_{std::move(coordination_client)},
      coordination_node_{coordination_node},
      semaphore_name_{semaphore_name},
      settings_{std::move(settings)},
      callback_{std::move(callback)},
      stats_{utils::MakeUniqueRef<impl::dist_lock::Statistics>()} {}

DistLockedWorker::~DistLockedWorker() {
  UASSERT_MSG(!worker_task_.IsValid(),
              "Forgot to call Stop() in the derived "
              "component's destructor");
}

void DistLockedWorker::Start() {
  LOG_DEBUG() << "Starting DistLockedWorker...";

  worker_task_ = utils::CriticalAsync(task_processor_, "dist-locked-worker",
                                      [this] { Run(/*run_once=*/false); });

  LOG_DEBUG() << "DistLockedWorker started";
}

void DistLockedWorker::Stop() noexcept {
  LOG_DEBUG() << "Stopping DistLockedWorker...";

  if (worker_task_.IsValid()) {
    worker_task_.RequestCancel();
    const engine::TaskCancellationBlocker cancel_blocker;
    try {
      worker_task_.Get();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Exception while stopping DistLockedWorker: " << ex;
    }
  }

  LOG_DEBUG() << "DistLockedWorker stopped";
}

void DistLockedWorker::RunOnce() { Run(/*run_once=*/true); }

void DistLockedWorker::Run(bool run_once) {
  while (!engine::current_task::ShouldCancel()) {
    try {
      LOG_INFO() << "Starting new session...";

      auto session_stopped_promise = std::make_shared<engine::Promise<void>>();
      auto session_stopped_future = session_stopped_promise->get_future();
      NYdb::NCoordination::TSessionSettings session_settings{};
      session_settings.Timeout(settings_.session_timeout);
      session_settings.OnStopped([session_stopped_promise = std::move(
                                      session_stopped_promise)]() mutable {
        session_stopped_promise->set_value();
      });
      auto session = coordination_client_->StartSession(coordination_node_,
                                                        session_settings);
      LOG_INFO() << "Session started";
      ++stats_->start_session_successes;

      SessionTask session_task{
          std::move(session), std::move(session_stopped_future),
          semaphore_name_,    callback_,
          settings_,          *stats_,
          owns_lock_};
      session_task.Run(run_once);

      LOG_INFO() << "Session finished";

      if (run_once) {
        return;
      }
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Start session error: " << ex;
      ++stats_->start_session_failures;
      engine::InterruptibleSleepFor(settings_.restart_session_delay);
    }
  }
}

bool DistLockedWorker::OwnsLock() const noexcept { return owns_lock_.load(); }

void DumpMetric(utils::statistics::Writer& writer,
                const DistLockedWorker& worker) {
  const auto& stats = *worker.stats_;

  writer["start-session-successes"] = stats.start_session_successes;
  writer["start-session-failures"] = stats.start_session_failures;

  writer["session-stopped"] = stats.session_stopped;

  writer["lock-successes"] = stats.lock_successes;
  writer["lock-failures"] = stats.lock_failures;

  writer["task-successes"] = stats.task_successes;
  writer["task-failures"] = stats.task_failures;

  writer["cancel-task-time-limit-exceeded"] =
      stats.cancel_task_time_limit_exceeded;
}

}  // namespace ydb

USERVER_NAMESPACE_END
