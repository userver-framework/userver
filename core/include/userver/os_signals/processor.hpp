#pragma once

/// @file userver/os_signals/processor.hpp
/// @brief @copybrief os_signals::Processor

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/os_signals/subscriber.hpp>
#include <userver/utils/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

#if !defined(__APPLE__)

/// @brief Constant for SIGUSR1
inline constexpr int kSigUsr1 = 10;

/// @brief Constant for SIGUSR2
inline constexpr int kSigUsr2 = 12;

#else
inline constexpr int kSigUsr1 = 30;
inline constexpr int kSigUsr2 = 31;
#endif

/// @ingroup userver_clients
///
/// @brief A client that allows to subscribe
/// to OS signals `SIGUSR1 and `SIGUSR2`.
///
/// Usually retrieved from os_signals::ProcessorComponent component. For tests
/// use os_signals::ProcessorMock.
///
/// @see @ref scripts/docs/en/userver/os_signals.md
class Processor final {
 public:
  explicit Processor(engine::TaskProcessor& task_processor);

  /// Listen for a specific signal `signum`. See the other overload for
  /// subscription to both signals.
  template <class Class>
  Subscriber AddListener(Class* obj, std::string_view name, int signum,
                         void (Class::*func)()) {
    auto execute = [obj, func, signum](int sig) {
      if (sig == signum) {
        (obj->*func)();
      }
    };

    return channel_.AddListener(concurrent::FunctionId(obj), name,
                                std::move(execute));
  }

  /// Listen for all the OS signals.
  template <class Class>
  Subscriber AddListener(Class* obj, std::string_view name,
                         void (Class::*func)(int /*signum*/)) {
    auto execute = [obj, func](int sig) { (obj->*func)(sig); };
    return channel_.AddListener(concurrent::FunctionId(obj), name,
                                std::move(execute));
  }

  /// @cond
  // For internal use
  void Notify(int signum, utils::InternalTag);
  /// @endcond
 private:
  concurrent::AsyncEventChannel<int> channel_;
  engine::TaskProcessor& task_processor_;
};

}  // namespace os_signals

USERVER_NAMESPACE_END
