#include <dist_lock/impl/locker.hpp>

#include <atomic>
#include <limits>
#include <stdexcept>
#include <utility>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/engine/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/rand.hpp>

#include <dist_lock/impl/helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {
namespace {

class WorkerFuncFailedException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string MakeLockerId(const std::string& name) {
    static std::atomic<uint32_t> idx = utils::Rand();
    return fmt::format(FMT_COMPILE("{}-{:x}"), name, idx++);
}

void ValidateSettings(const DistLockSettings& settings) {
    if (!settings.fallback_policy) {
        if (settings.is_host_deprioritized) {
            throw std::invalid_argument("Deprioritized distlock hosts require a fallback policy");
        }
        return;
    }
    const auto& fallback = *settings.fallback_policy;
    if (fallback.fallback_after_checks == 0) {
        throw std::invalid_argument("Distlock fallback_after_checks must be positive");
    }
    if (fallback.fallback_duration.count() <= 0) {
        throw std::invalid_argument("Distlock fallback_duration must be positive");
    }
    if (settings.acquire_interval.count() <= 0) {
        throw std::invalid_argument("Distlock acquire_interval must be positive when fallback is enabled");
    }
    const auto max_idle_ms = std::numeric_limits<std::chrono::milliseconds::rep>::max();
    const auto max_checks = max_idle_ms / settings.acquire_interval.count();
    if (fallback.fallback_after_checks > static_cast<std::size_t>(max_checks)) {
        throw std::invalid_argument(fmt::format(
            "Distlock fallback_after_checks={} exceeds the maximum of {} for acquire_interval={}ms "
            "(maximum idle interval is {}ms)",
            fallback.fallback_after_checks,
            max_checks,
            settings.acquire_interval.count(),
            max_idle_ms
        ));
    }
}

}  // namespace

class Locker::LockGuard {
public:
    LockGuard(Locker& locker)
        : locker_(locker)
    {}
    ~LockGuard() {
        TryUnlock();
        locker_.ResetFallback();
    }

    void TryUnlock() noexcept {
        const engine::TaskCancellationBlocker cancel_blocker;
        try {
            if (locker_.ExchangeLockState(false, utils::datetime::SteadyNow())) {
                locker_.ReleaseLock();
            }
        } catch (const std::exception& ex) {
            LOG_WARNING() << "Failed to release lock on stop: " << ex;
        }
    }

private:
    Locker& locker_;
};

Locker::Locker(
    std::string name,
    std::shared_ptr<DistLockStrategyBase> strategy,
    const DistLockSettings& settings,
    std::function<void()> worker_func,
    DistLockRetryMode retry_mode,
    logging::Level base_log_level
)
    : name_(std::move(name)),
      id_(MakeLockerId(name_)),
      strategy_(std::move(strategy)),
      worker_func_(std::move(worker_func)),
      settings_(settings),
      retry_mode_(retry_mode),
      base_log_level_(base_log_level)
{
    UASSERT(strategy_);
    ValidateSettings(settings_);
}

const std::string& Locker::Name() const { return name_; }

const std::string& Locker::Id() const { return id_; }

DistLockSettings Locker::GetSettings() const {
    const std::lock_guard<engine::Mutex> lock(state_mutex_);
    return settings_;
}

void Locker::ReleaseLock() { ReleaseLock(std::unique_lock<engine::Mutex>{state_mutex_}); }

void Locker::ReleaseLock(std::unique_lock<engine::Mutex>&& lock) {
    const ReleaseOptions options{.preserve_history = settings_.fallback_policy.has_value()};
    lock.unlock();
    strategy_->Release(Id(), options);
}

void Locker::ResetFallback() {
    const std::lock_guard<engine::Mutex> lock(state_mutex_);
    ResetFallback(lock);
}

void Locker::ResetFallback(const std::lock_guard<engine::Mutex>&) noexcept {
    negative_heartbeats_ = 0;
    fallback_until_.store({});
}

void Locker::SetSettings(const DistLockSettings& settings) {
    ValidateSettings(settings);
    const std::lock_guard<engine::Mutex> lock(state_mutex_);
    if (settings_ == settings) {
        return;
    }
    settings_ = settings;
    ResetFallback(lock);
}

bool Locker::IsCancelAdvised() const {
    const std::lock_guard<engine::Mutex> lock(state_mutex_);
    if (!settings_.is_enabled) {
        return true;
    }
    if (!settings_.is_host_deprioritized) {
        return false;
    }
    return utils::datetime::SteadyNow().time_since_epoch() >= fallback_until_.load();
}

LockOperation Locker::GetLockOperation(const DistLockSettings& settings, std::chrono::steady_clock::time_point now)
    const {
    if (is_locked_) {
        return LockOperation::kProlong;
    }
    if (settings.is_host_deprioritized && now.time_since_epoch() >= fallback_until_.load()) {
        return LockOperation::kFallback;
    }
    return LockOperation::kAcquire;
}

LockOperationResult Locker::TryFallback(const DistLockSettings& settings) {
    const auto
        result = strategy_->ExecuteOperation(Id(), {.lock_ttl = settings.lock_ttl, .operation = LockOperation::kCheck});
    UINVARIANT(result != LockOperationResult::kAcquired, "A liveness check must not acquire the lock");
    const auto& fallback = settings.fallback_policy.value();
    {
        const std::lock_guard<engine::Mutex> lock(state_mutex_);
        if (result != LockOperationResult::kNotAlive || settings != settings_) {
            ResetFallback(lock);
            return result;
        }
        if (++negative_heartbeats_ < fallback.fallback_after_checks) {
            return LockOperationResult::kNotAlive;
        }
        ResetFallback(lock);
    }
    const auto checks = static_cast<std::chrono::milliseconds::rep>(fallback.fallback_after_checks);
    return strategy_->ExecuteOperation(
        Id(),
        {.lock_ttl = settings.lock_ttl,
         .operation = LockOperation::kFallback,
         .min_idle_time_milliseconds = settings.acquire_interval * checks}
    );
}

LockOperationResult Locker::CompleteAcquisition(
    const DistLockSettings& settings,
    LockOperation operation,
    std::chrono::steady_clock::time_point now
) {
    UASSERT(operation == LockOperation::kAcquire || operation == LockOperation::kFallback);
    std::unique_lock<engine::Mutex> lock(state_mutex_);
    if (settings == settings_) {
        if (operation == LockOperation::kFallback) {
            fallback_until_ = (now + settings.fallback_policy.value().fallback_duration).time_since_epoch();
        }
        return LockOperationResult::kAcquired;
    }
    ReleaseLock(std::move(lock));
    return LockOperationResult::kNotAlive;
}

LockOperationResult Locker::TryLockOperation(
    const DistLockSettings& settings,
    LockOperation operation,
    std::chrono::steady_clock::time_point now
) {
    UASSERT(operation != LockOperation::kCheck);
    try {
        auto result = LockOperationResult::kUnsupported;
        if (operation == LockOperation::kFallback) {
            result = TryFallback(settings);
        } else {
            result = strategy_->ExecuteOperation(Id(), {.lock_ttl = settings.lock_ttl, .operation = operation});
        }
        if (result == LockOperationResult::kAcquired) {
            if (is_locked_) {
                return LockOperationResult::kAcquired;
            }
            return CompleteAcquisition(settings, operation, now);
        }
        if (operation != LockOperation::kFallback) {
            ResetFallback();
        }
        return result;
    } catch (const std::exception&) {
        ResetFallback();
        throw;
    }
}

std::optional<std::chrono::steady_clock::duration> Locker::GetLockedDuration() const {
    if (!is_locked_) {
        return {};
    }
    return std::chrono::steady_clock::now().time_since_epoch() - lock_acquire_since_epoch_.load();
}

const Statistics& Locker::GetStatistics() const { return stats_; }

void Locker::HandleBusyLock(engine::TaskWithResult<void> watchdog_task) {
    LOG(base_log_level_) << "Fail to acquire lock. It was acquired by another host";
    stats_.lock_failures++;
    if (!is_locked_) {
        return;
    }
    LOG_ERROR()
        << "DistLockedTask brain split detected! Someone else acquired the "
           "lock while we're assuming we're holding the lock. It may be "
           "a brain split in DB backend or missing cancellation point in "
           "worker code.";
    stats_.brain_splits++;
    LOG_DEBUG() << "Terminating watchdog task";
    if (watchdog_task.IsValid()) {
        watchdog_task.RequestCancel();
    }
    GetTask(watchdog_task, name_, "cancel and watchdog wait after brain split detected");
    LOG_DEBUG() << "Terminated watchdog task";
    ExchangeLockState(false, utils::datetime::SteadyNow());
}

void Locker::Run(LockerMode mode, dist_lock::DistLockWaitingMode waiting_mode, tracing::Span&& span) {
    span.AttachToCoroStack();
    LockGuard lock_guard(*this);
    engine::TaskWithResult<void> watchdog_task;
    bool worker_succeeded = false;
    bool unsupported_reported = false;

    while (!engine::current_task::ShouldCancel() && (mode != LockerMode::kOneshot || !worker_succeeded)) {
        const auto settings = GetSettings();
        const auto start_time = utils::datetime::SteadyNow();

        try {
            if (settings.is_enabled || is_locked_) {
                const auto operation = GetLockOperation(settings, start_time);
                const auto result = TryLockOperation(settings, operation, start_time);
                if (result == LockOperationResult::kAcquired) {
                    stats_.lock_successes++;
                    if (!ExchangeLockState(true, start_time)) {
                        LOG_DEBUG() << "Starting watchdog task";
                        GetTask(watchdog_task, name_, "watchdog wait stale task");
                        watchdog_task = utils::CriticalAsync(WatchdogName(name_), [this] { RunWatchdog(); });
                        LOG_DEBUG() << "Started watchdog task";
                    }
                } else if (result == LockOperationResult::kBusy || is_locked_) {
                    HandleBusyLock(std::move(watchdog_task));
                    if (waiting_mode == dist_lock::DistLockWaitingMode::kNoWait) {
                        break;
                    }
                } else if (result == LockOperationResult::kUnsupported && !unsupported_reported) {
                    LOG_WARNING() << "Distlock strategy does not support fallback checks";
                    unsupported_reported = true;
                }
            } else if (mode == LockerMode::kOneshot) {
                // To prevent going to next iteration since worker_succeeded is `false`
                break;
            }
        } catch (const std::exception& ex) {
            ResetFallback();
            stats_.lock_failures++;
            LOG_WARNING() << "Lock acquisition failed: " << ex;
        }

        if (engine::current_task::ShouldCancel()) {
            break;
        }

        auto delay = is_locked_ ? settings.prolong_interval : settings.acquire_interval;
        if (watchdog_task.IsValid()) {
            try {
                watchdog_task.WaitFor(delay);
            } catch (const engine::WaitInterruptedException&) {
                // do nothing
            }

            if (watchdog_task.IsFinished()) {
                std::exception_ptr exception;
                worker_succeeded = GetTask(watchdog_task, name_, "watchdog wait", &exception);
                if (!worker_succeeded) {
                    stats_.task_failures++;
                }
                if (!worker_succeeded || mode == LockerMode::kWorker) {
                    lock_guard.TryUnlock();
                    if (retry_mode_ == DistLockRetryMode::kSingleAttempt) {
                        if (exception) {
                            std::rethrow_exception(exception);
                        } else {
                            throw WorkerFuncFailedException{
                                fmt::format("worker name='{}' id='{}' task failed", name_, id_)
                            };
                        }
                    }
                    engine::InterruptibleSleepFor(settings.worker_func_restart_delay);
                }
            }
        } else {
            engine::InterruptibleSleepFor(delay);
        }
    }
    if (watchdog_task.IsValid()) {
        watchdog_task.RequestCancel();
    }
    GetTask(watchdog_task, name_, "cancel and watchdog wait on finish");
}

engine::TaskWithResult<void> Locker::RunAsync(
    engine::TaskProcessor& task_processor,
    LockerMode locker_mode,
    DistLockWaitingMode waiting_mode
) {
    tracing::Span span(impl::LockerName(Name()));
    span.DetachFromCoroStack();

    return engine::CriticalAsyncNoTracing(
        task_processor,
        [this, locker_mode, waiting_mode, span = std::move(span)]() mutable {
            Run(locker_mode, waiting_mode, std::move(span));
        }
    );
}

bool Locker::OwnsLock() const noexcept { return is_locked_.load(); }

bool Locker::ExchangeLockState(bool is_locked, std::chrono::steady_clock::time_point when) {
    lock_refresh_since_epoch_.store(when.time_since_epoch(), std::memory_order_release);

    auto was_locked = is_locked_.exchange(is_locked);
    if (is_locked != was_locked) {
        if (!was_locked) {
            LOG(base_log_level_) << "Acquired the lock";
            lock_acquire_since_epoch_ = when.time_since_epoch();
        } else {
            LOG(base_log_level_) << "Released (or lost) the lock";
        }
    }
    return was_locked;
}

void Locker::RunWatchdog() {
    LOG(base_log_level_) << "Starting worker task '" << name_ << "'";
    auto worker_task = utils::CriticalAsync("lock-worker-" + name_, [this] { worker_func_(); });
    LOG(base_log_level_) << "Started worker task '" << name_ << "'";

    bool failed_to_prolong = false;
    while (!engine::current_task::ShouldCancel() && !worker_task.IsFinished()) {
        const auto settings = GetSettings();

        const auto refresh_since_epoch = lock_refresh_since_epoch_.load(std::memory_order_acquire);
        const auto deadline =
            std::chrono::steady_clock::time_point{refresh_since_epoch} + settings.lock_ttl -
            settings.forced_stop_margin;
        const auto now = utils::datetime::SteadyNow();

        failed_to_prolong = (deadline < now);
        if (failed_to_prolong) {
            LOG_ERROR()
                << "Failed to prolong the lock before the deadline, "
                   "voluntarily dropping the lock and killing the worker "
                   "task '"
                << name_ << "' to avoid brain split";
            stats_.watchdog_triggers++;
            break;
        } else {
            LOG_DEBUG()
                << "Watchdog found a valid locked timepoint (" << now.time_since_epoch().count() << " < "
                << deadline.time_since_epoch().count() << ") for task '" << name_ << "'";
        }

        try {
            worker_task.WaitFor(settings.forced_stop_margin / 2);
        } catch (const engine::WaitInterruptedException&) {
            // do nothing
        }
    }
    LOG(base_log_level_) << "Waiting for worker task '" << name_ << "'";

    std::string_view error_context = "wait";
    if (!worker_task.IsFinished()) {
        worker_task.RequestCancel();  // do not call on OK task to report exceptions as ERRORs
        error_context =
            (failed_to_prolong ? "cancel and wait after prolongation fail" : "cancel from parent task and wait");
    }

    std::exception_ptr exception;
    if (!GetTask(worker_task, name_, error_context, &exception)) {
        LOG_WARNING() << "Worker task '" << name_ << "' failed";
        // throw exception to report to Run() that the task failed
        if (exception) {
            std::rethrow_exception(exception);
        } else {
            throw WorkerFuncFailedException{fmt::format("worker name='{}' id='{}' task failed", name_, id_)};
        }
    }

    LOG(base_log_level_) << "Worker task '" << name_ << "' completed";
}

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
