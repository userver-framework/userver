#pragma once

/// @file userver/concurrent/async_event_source.hpp
/// @brief @copybrief concurrent::AsyncEventSource

#include <cstddef>
#include <functional>
#include <string_view>
#include <typeinfo>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

class FunctionId;
enum class UnsubscribingKind;

template <typename... Args>
class AsyncEventSource;

namespace impl {

class AsyncEventSourceBase {
 public:
  virtual void RemoveListener(FunctionId, UnsubscribingKind) noexcept = 0;

 protected:
  // disallow deletion via pointer to base
  ~AsyncEventSourceBase();
};

}  // namespace impl

/// Uniquely identifies a subscription (usually the callback method owner) for
/// AsyncEventSource
class FunctionId final {
 public:
  constexpr FunctionId() = default;

  template <typename Class>
  explicit FunctionId(Class* obj) : ptr_(obj), type_(&typeid(Class)) {}

  explicit operator bool() const;

  bool operator==(const FunctionId& other) const;

  struct Hash final {
    std::size_t operator()(FunctionId id) const noexcept;
  };

 private:
  void* ptr_{nullptr};
  const std::type_info* type_{nullptr};
};

enum class UnsubscribingKind { kManual, kAutomatic };

/// @brief Manages the subscription to events from an AsyncEventSource
///
/// Removes the associated listener automatically on destruction.
///
/// The Scope is usually placed as a member in the subscribing object.
/// `Unsubscribe` should be called manually in the objects destructor, before
/// anything that the callback needs is destroyed.
class [[nodiscard]] AsyncEventSubscriberScope final {
 public:
  AsyncEventSubscriberScope() = default;

  template <typename... Args>
  AsyncEventSubscriberScope(AsyncEventSource<Args...>& channel, FunctionId id)
      : AsyncEventSubscriberScope(
            static_cast<impl::AsyncEventSourceBase&>(channel), id) {}

  AsyncEventSubscriberScope(AsyncEventSubscriberScope&& scope) noexcept;

  AsyncEventSubscriberScope& operator=(
      AsyncEventSubscriberScope&& other) noexcept;

  ~AsyncEventSubscriberScope();

  /// Unsubscribes manually. The subscription should be cancelled before
  /// anything that the callback needs is destroyed.
  void Unsubscribe() noexcept;

 private:
  AsyncEventSubscriberScope(impl::AsyncEventSourceBase& channel, FunctionId id);

  impl::AsyncEventSourceBase* channel_{nullptr};
  FunctionId id_;
};

/// @ingroup userver_concurrency
///
/// @brief The read-only side of an event channel. Events are delivered to
/// listeners in a strict FIFO order, i.e. only after the event was processed
/// a new event may appear for processing, same listener is never called
/// concurrently.
template <typename... Args>
class AsyncEventSource : public impl::AsyncEventSourceBase {
 public:
  using Function = std::function<void(Args... args)>;

  virtual ~AsyncEventSource() = default;

  /// @brief Subscribes to updates from this event source
  ///
  /// The listener won't be called immediately. To process the current value and
  /// then listen to updates, use `UpdateAndListen` of specific event channels.
  ///
  /// @warning Listeners should not be added or removed while processing the
  /// event inside another listener.
  ///
  /// Example usage:
  /// @snippet concurrent/async_event_channel_test.cpp  AddListener sample
  ///
  /// @param obj the subscriber, which is the owner of the listener method, and
  /// is also used as the unique identifier of the subscription for this
  /// AsyncEventSource
  /// @param name the name of the subscriber, for diagnostic purposes
  /// @param func the listener method, usually called `On<DataName>Update`, e.g.
  /// `OnConfigUpdate` or `OnCacheUpdate`
  /// @returns a AsyncEventSubscriberScope controlling the subscription, which
  /// should be stored as a member in the subscriber; `Unsubscribe` should be
  /// called explicitly
  template <class Class>
  AsyncEventSubscriberScope AddListener(Class* obj, std::string_view name,
                                        void (Class::*func)(Args...)) {
    return AddListener(FunctionId(obj), name,
                       [obj, func](Args... args) { (obj->*func)(args...); });
  }

  /// @brief A type-erased version of AddListener
  ///
  /// @param id the unique identifier of the subscription
  /// @param name the name of the subscriber, for diagnostic purposes
  /// @param func the callback used for event notifications
  AsyncEventSubscriberScope AddListener(FunctionId id, std::string_view name,
                                        Function&& func) {
    return DoAddListener(id, name, std::move(func));
  }

  /// Used by AsyncEventSubscriberScope to cancel an event subscription
  void RemoveListener(FunctionId, UnsubscribingKind) noexcept override = 0;

 protected:
  virtual AsyncEventSubscriberScope DoAddListener(FunctionId id,
                                                  std::string_view name,
                                                  Function&& func) = 0;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
