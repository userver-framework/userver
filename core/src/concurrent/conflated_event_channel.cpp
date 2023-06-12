#include <userver/concurrent/conflated_event_channel.hpp>
#include <userver/utils/async.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

ConflatedEventChannel::ConflatedEventChannel(
    std::string name, OnRemoveCallback on_listener_removal)
    : AsyncEventChannel<>(std::move(name), on_listener_removal),
      stop_flag_(false) {
  LOG_DEBUG() << Name() << ": start event listener task";
  task_ = utils::Async(Name() + "/event_listener", [this] {
    while (event_.WaitForEvent()) {
      if (stop_flag_) break;
      LOG_TRACE() << Name() << ": send event";
      try {
        AsyncEventChannel<>::SendEvent();
      } catch (const std::exception& e) {
        LOG_ERROR() << Name() << ": unexpected exception in SendEvent: " << e;
      }
    }
    LOG_DEBUG() << Name() << ": event listener task ends";
  });
}

ConflatedEventChannel::~ConflatedEventChannel() {
  LOG_DEBUG() << Name() << ": stop event listener task";
  for (auto& s : subscriptions_) {
    s.Unsubscribe();
  }
  subscriptions_.clear();

  stop_flag_ = true;
  event_.Send();
  try {
    task_.Get();
  } catch (const std::exception& e) {
    LOG_ERROR() << Name() << ": unexpected exception in task_.Get: " << e;
  }
}

void ConflatedEventChannel::SendEvent() {
  LOG_TRACE() << Name() << ": channel event received";
  event_.Send();
  if (task_.IsFinished()) {
    throw std::runtime_error(Name() + ": event listener died unexpectedly");
  }
}

}  // namespace concurrent

USERVER_NAMESPACE_END
