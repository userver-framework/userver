#pragma once

#include <atomic>
#include <vector>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency
///
/// @brief A non-blocking version of 'AsyncEventChannel'
///
/// The difference is that 'SendEvent' returns immediately, without waiting for
/// subscribers to finish. If 'SendEvent' is called multiple times while
/// subscribers are handling the previous event, new events will be conflated
/// (all events except for the last one will be ignored). This class can be used
/// instead of 'AsyncEventChannel' when we've got a "heavy" subscriber and we
/// don't want to slow down the pipeline.
class ConflatedEventChannel : private AsyncEventChannel<> {
 public:
  explicit ConflatedEventChannel(std::string name,
                                 OnRemoveCallback on_listener_removal = {});
  ~ConflatedEventChannel() override;

  /// For convenient forwarding of events from other channels
  template <typename... Args>
  void AddChannel(concurrent::AsyncEventSource<Args...>& channel);

  using AsyncEventChannel<>::AddListener;

  /// Subscribes to updates using a member function. Also immediately invokes
  /// the function with the current config snapshot.
  template <typename Class>
  concurrent::AsyncEventSubscriberScope UpdateAndListen(Class* obj,
                                                        std::string_view name,
                                                        void (Class::*func)());

  void SendEvent();

 private:
  template <typename... Args>
  void OnChannelEvent(Args...);

  std::atomic<bool> stop_flag_;
  engine::TaskWithResult<void> task_;
  std::vector<concurrent::AsyncEventSubscriberScope> subscriptions_;
  engine::SingleConsumerEvent event_;
};

template <typename... Args>
void ConflatedEventChannel::AddChannel(
    concurrent::AsyncEventSource<Args...>& channel) {
  subscriptions_.push_back(channel.AddListener(
      this, Name(), &ConflatedEventChannel::OnChannelEvent<Args...>));
}

template <typename Class>
concurrent::AsyncEventSubscriberScope ConflatedEventChannel::UpdateAndListen(
    Class* obj, std::string_view name, void (Class::*func)()) {
  return DoUpdateAndListen(obj, name, func, [&] { (obj->*func)(); });
}

template <typename... Args>
void ConflatedEventChannel::OnChannelEvent(Args...) {
  SendEvent();
}

}  // namespace concurrent

USERVER_NAMESPACE_END
