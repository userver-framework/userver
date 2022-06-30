#pragma once

/// @file userver/os_signals/processor.hpp
/// @brief @copybrief os_signals::Processor

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/os_signals/subscriber.hpp>
#include <userver/utils/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

/// @brief A manager that allows to subscribe
/// to OS signals, example: SIGUSR1, SIGUSR2
class Processor final {
 public:
  explicit Processor(engine::TaskProcessor& task_processor);

  template <class Class>
  Subscriber AddListener(Class* obj, std::string_view name, int signum,
                         void (Class::*func)()) {
    auto execute = [obj, func, signum](int sig) {
      if (sig == signum) {
        (obj->*func)();
      }
    };

    return {channel_.AddListener(concurrent::FunctionId(obj), name,
                                 std::move(execute))};
  }

  /// @cond
  void Notify(int signum, utils::InternalTag);
  /// @endcond

 private:
  concurrent::AsyncEventChannel<int> channel_;
  engine::TaskProcessor& task_processor_;
};

}  // namespace os_signals

USERVER_NAMESPACE_END
