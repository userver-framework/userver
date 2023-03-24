#include <userver/concurrent/async_event_channel.hpp>

#include <chrono>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

void WaitForTask(std::string_view name, engine::TaskWithResult<void>& task) {
  constexpr std::chrono::seconds kSubscriberTimeout(30);

  while (true) {
    task.WaitFor(kSubscriberTimeout);
    if (task.IsFinished()) break;

    LOG_ERROR() << "Subscriber " << name << " handles event for too long";
  }

  try {
    task.Get();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Unhandled exception in subscriber " << name << ": " << e;
  }
}

[[noreturn]] void ReportAlreadySubscribed(std::string_view channel_name,
                                          std::string_view listener_name) {
  UINVARIANT(false, fmt::format("{} is already subscribed to channel {}",
                                listener_name, channel_name));
}

void ReportNotSubscribed(std::string_view channel_name) noexcept {
  // RemoveListener is called from destructors, don't throw in production
  LOG_ERROR() << "Trying to unregister a subscriber which is not registered "
                 "for channel "
              << channel_name
              << ". Stacktrace: " << logging::LogExtra::Stacktrace();
  UASSERT_MSG(false,
              "Trying to unregister a subscriber which is not registered");
}

void ReportUnsubscribingAutomatically(std::string_view channel_name,
                                      std::string_view listener_name) noexcept {
  LOG_DEBUG() << "Listener " << listener_name
              << " is unsubscribing automatically from channel " << channel_name
              << ", which can invoke UB. Please call 'Unsubscribe' manually in "
                 "destructors.";
}

void ReportErrorWhileUnsubscribing(std::string_view channel_name,
                                   std::string_view listener_name,
                                   std::string_view error) noexcept {
  LOG_ERROR() << "Unhandled exception while listener " << listener_name
              << " is unsubscribing automatically from channel " << channel_name
              << ": " << error;
}

std::string MakeAsyncChannelName(std::string_view base, std::string_view name) {
  return fmt::format("async_channel/{}_{}", base, name);
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
