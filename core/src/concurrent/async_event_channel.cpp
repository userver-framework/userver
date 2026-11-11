#include <userver/concurrent/async_event_channel.hpp>

#include <chrono>
#include <mutex>
#include <shared_mutex>

#include <fmt/format.h>

#include <userver/engine/shared_mutex.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

std::shared_lock<engine::SharedMutex> DowngradeLock(std::unique_lock<engine::SharedMutex> lock) {
    UASSERT(lock.owns_lock());
    auto& mutex = *lock.release();
    mutex.unlock_and_lock_shared();
    return std::shared_lock{mutex, std::adopt_lock};
}

void WaitForTask(std::string_view name, engine::TaskWithResult<void>& task) {
    constexpr std::chrono::seconds kSubscriberTimeout(30);

    while (true) {
        task.WaitFor(kSubscriberTimeout);
        if (task.IsFinished()) {
            break;
        }

        LOG_ERROR() << "Subscriber " << name << " handles event for too long";
    }

    try {
        task.Get();
    } catch (const std::exception& e) {
        LOG_ERROR() << "Unhandled exception in subscriber " << name << ": " << e;
    }
}

[[noreturn]] void ReportAlreadySubscribed(std::string_view channel_name, std::string_view listener_name) {
    UINVARIANT(false, fmt::format("{} is already subscribed to channel {}", listener_name, channel_name));
}

void ReportNotSubscribed(std::string_view channel_name) noexcept {
    // RemoveListener is called from destructors, don't throw in production
    LOG_ERROR()
        << "Trying to unregister a subscriber which is not registered "
           "for channel "
        << channel_name << ". Stacktrace: " << logging::LogExtra::Stacktrace();
    UASSERT_MSG(false, "Trying to unregister a subscriber which is not registered");
}

void ReportErrorWhileUnsubscribing(
    std::string_view channel_name,
    std::string_view listener_name,
    std::string_view error
) noexcept {
    LOG_ERROR()
        << "Unhandled exception while listener " << listener_name << " is unsubscribing from channel " << channel_name
        << ": " << error;
}

std::string MakeAsyncChannelName(std::string_view base, std::string_view name) {
    return fmt::format("async_channel/{}_{}", base, name);
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
