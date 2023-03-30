#pragma once

/// @file userver/concurrent/async_event_channel.hpp
/// @brief @copybrief concurrent::AsyncEventChannel

#include <functional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <userver/concurrent/async_event_source.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

void WaitForTask(std::string_view name, engine::TaskWithResult<void>& task);

[[noreturn]] void ReportAlreadySubscribed(std::string_view channel_name,
                                          std::string_view listener_name);

void ReportNotSubscribed(std::string_view channel_name) noexcept;

void ReportUnsubscribingAutomatically(std::string_view channel_name,
                                      std::string_view listener_name) noexcept;

void ReportErrorWhileUnsubscribing(std::string_view channel_name,
                                   std::string_view listener_name,
                                   std::string_view error) noexcept;

std::string MakeAsyncChannelName(std::string_view base, std::string_view name);

inline constexpr bool kCheckSubscriptionUB = utils::impl::kEnableAssert;

// During the `AsyncEventSubscriberScope::Unsubscribe` call or destruction of
// `AsyncEventSubscriberScope`, all variables used by callback must be valid
// (must not be destroyed). A common cause of crashes in this place: there is no
// manual call to `Unsubscribe`. In this case check the declaration order of the
// struct fields.
template <typename Func>
void CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnsubscribing(
    std::function<void(Func&)>& on_listener_removal, Func& listener_func,
    std::string_view channel_name, std::string_view listener_name) noexcept {
  if (!on_listener_removal) return;
  try {
    on_listener_removal(listener_func);
  } catch (const std::exception& e) {
    ReportErrorWhileUnsubscribing(channel_name, listener_name, e.what());
  }
}

}  // namespace impl

/// @ingroup userver_concurrency
///
/// AsyncEventChannel is an in-process pub-sub with strict FIFO serialization,
/// i.e. only after the event was processed a new event may appear for
/// processing, same listener is never called concurrently.
///
/// Example usage:
/// @snippet concurrent/async_event_channel_test.cpp  AsyncEventChannel sample
template <typename... Args>
class AsyncEventChannel : public AsyncEventSource<Args...> {
 public:
  using Function = typename AsyncEventSource<Args...>::Function;
  using OnRemoveCallback = std::function<void(Function&)>;

  /// @brief The primary constructor
  /// @param name used for diagnostic purposes and is also accessible with Name
  explicit AsyncEventChannel(std::string name,
                             OnRemoveCallback on_listener_removal = {})
      : name_(std::move(name)),
        data_(ListenersData{{}, std::move(on_listener_removal)}) {}

  /// @brief For use in `UpdateAndListen` of specific event channels
  ///
  /// Atomically calls `updater`, which should invoke `func` with the previously
  /// sent event, and subscribes to new events as if using AddListener.
  ///
  /// @see AsyncEventSource::AddListener
  template <typename UpdaterFunc>
  AsyncEventSubscriberScope DoUpdateAndListen(FunctionId id,
                                              std::string_view name,
                                              Function&& func,
                                              UpdaterFunc&& updater) {
    std::lock_guard lock(event_mutex_);
    std::forward<UpdaterFunc>(updater)();
    return DoAddListener(id, name, std::move(func));
  }

  /// @overload
  template <typename Class, typename UpdaterFunc>
  AsyncEventSubscriberScope DoUpdateAndListen(Class* obj, std::string_view name,
                                              void (Class::*func)(Args...),
                                              UpdaterFunc&& updater) {
    return DoUpdateAndListen(
        FunctionId(obj), name,
        [obj, func](Args... args) { (obj->*func)(args...); },
        std::forward<UpdaterFunc>(updater));
  }

  /// Send the next event and wait until all the listeners process it.
  ///
  /// Strict FIFO serialization is guaranteed, i.e. only after this event is
  /// processed a new event may be delivered for the subscribers, same
  /// listener/subscriber is never called concurrently.
  void SendEvent(Args... args) const {
    std::lock_guard lock(event_mutex_);
    auto data = data_.Lock();
    auto& listeners = data->listeners;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(listeners.size());

    for (const auto& [_, listener] : listeners) {
      tasks.push_back(utils::Async(
          listener.task_name,
          [&, &callback = listener.callback] { callback(args...); }));
    }

    std::size_t i = 0;
    for (const auto& [_, listener] : listeners) {
      impl::WaitForTask(listener.name, tasks[i++]);
    }
  }

  /// @returns the name of this event channel
  const std::string& Name() const noexcept { return name_; }

 private:
  struct Listener final {
    std::string name;
    Function callback;
    std::string task_name;
  };

  struct ListenersData final {
    std::unordered_map<FunctionId, Listener, FunctionId::Hash> listeners;
    OnRemoveCallback on_listener_removal;
  };

  void RemoveListener(FunctionId id, UnsubscribingKind kind) noexcept final {
    engine::TaskCancellationBlocker blocker;
    auto data = data_.Lock();
    auto& listeners = data->listeners;
    const auto iter = listeners.find(id);

    if (iter == listeners.end()) {
      impl::ReportNotSubscribed(Name());
      return;
    }

    if (kind == UnsubscribingKind::kAutomatic) {
      if (!data->on_listener_removal) {
        impl::ReportUnsubscribingAutomatically(name_, iter->second.name);
      }

      if constexpr (impl::kCheckSubscriptionUB) {
        // Fake listener call to check
        impl::CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnsubscribing(
            data->on_listener_removal, iter->second.callback, name_,
            iter->second.name);
      }
    }
    listeners.erase(iter);
  }

  AsyncEventSubscriberScope DoAddListener(FunctionId id, std::string_view name,
                                          Function&& func) final {
    auto data = data_.Lock();
    auto& listeners = data->listeners;
    auto task_name = impl::MakeAsyncChannelName(name_, name);
    const auto [iterator, success] = listeners.emplace(
        id, Listener{std::string{name}, std::move(func), std::move(task_name)});
    if (!success) impl::ReportAlreadySubscribed(Name(), name);
    return AsyncEventSubscriberScope(*this, id);
  }

  const std::string name_;
  concurrent::Variable<ListenersData> data_;
  mutable engine::Mutex event_mutex_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
