#include <userver/concurrent/async_event_channel.hpp>

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

void WaitForTask(const std::string& name, engine::TaskWithResult<void>& task) {
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

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
