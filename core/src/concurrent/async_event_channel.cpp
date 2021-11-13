#include <userver/concurrent/async_event_channel.hpp>

#include <chrono>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

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

std::string MakeAsyncChanelName(std::string_view base, std::string_view name) {
  return fmt::format("async_channel/{}_{}", base, name);
}

}  // namespace impl

AsyncEventChannelBase::~AsyncEventChannelBase() = default;

}  // namespace concurrent

USERVER_NAMESPACE_END
