#include <dist_lock/impl/helpers.hpp>

#include <fmt/format.h>

#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {

bool GetTask(
    engine::TaskWithResult<void>& task,
    std::string_view name,
    std::string_view error_context,
    std::exception_ptr* exception_ptr
) {
    if (!task.IsValid()) {
        return false;
    }

    const bool task_was_not_cancelled = (task.CancellationReason() == engine::TaskCancellationReason::kNone);
    try {
        const engine::TaskCancellationBlocker cancel_blocker{};
        task.Get();
        return true;
    } catch (const engine::TaskCancelledException& e) {
        // Do nothing
    } catch (const std::exception& e) {
        const auto log_level = (task_was_not_cancelled ? logging::Level::kError : logging::Level::kWarning);
        LOG(log_level) << "Unexpected exception on " << name << " task during " << error_context << ": " << e;
        if (exception_ptr) {
            *exception_ptr = std::current_exception();
        }
    }
    return false;
}

std::string LockerName(std::string_view lock_name) { return fmt::format("locker-{}", lock_name); }

std::string WatchdogName(std::string_view lock_name) { return fmt::format("watchdog-{}", lock_name); }

std::string WorkerName(std::string_view lock_name) { return fmt::format("lock-worker-{}", lock_name); }

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
