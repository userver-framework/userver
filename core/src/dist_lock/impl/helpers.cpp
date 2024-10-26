#include <dist_lock/impl/helpers.hpp>

#include <fmt/format.h>

#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {

bool GetTask(
    engine::TaskWithResult<void>& task,
    std::string_view name,
    std::exception_ptr* exception_ptr,
    utils::impl::SourceLocation location
) {
    try {
        engine::TaskCancellationBlocker cancel_blocker;
        if (task.IsValid()) {
            task.Get();
            return true;
        }
    } catch (const engine::TaskCancelledException& e) {
        // Do nothing
    } catch (const std::exception& e) {
        if (logging::ShouldLog(logging::Level::kError)) {
            logging::LogHelper lh{logging::GetDefaultLogger(), logging::Level::kError, location};
            lh << "Unexpected exception on " << name << ".Get(): " << e;
        }
        if (exception_ptr) *exception_ptr = std::current_exception();
    }
    return false;
}

std::string LockerName(std::string_view lock_name) { return fmt::format("locker-{}", lock_name); }

std::string WatchdogName(std::string_view lock_name) { return fmt::format("watchdog-{}", lock_name); }

std::string WorkerName(std::string_view lock_name) { return fmt::format("lock-worker-{}", lock_name); }

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
