#pragma once

/// @file userver/concurrent/async_event_source.hpp
/// @brief @copybrief concurrent::AsyncEventSource

#include <cstddef>
#include <functional>
#include <string_view>
#include <typeinfo>
#include <utility>

#include <userver/utils/clang_format_workarounds.hpp>

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

class USERVER_NODISCARD AsyncEventSubscriberScope final {
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

  void Unsubscribe() noexcept;

 private:
  AsyncEventSubscriberScope(impl::AsyncEventSourceBase& channel, FunctionId id);

  impl::AsyncEventSourceBase* channel_{nullptr};
  FunctionId id_;
};

/// @ingroup userver_concurrency
///
/// @brief The read-only side of an event channel. Events are delivered to
/// listeners in a strict FIFO order.
template <typename... Args>
class AsyncEventSource : public impl::AsyncEventSourceBase {
 public:
  using Function = std::function<void(Args... args)>;

  virtual ~AsyncEventSource() = default;

  /// Subscribes to updates from this event source. The subscription is
  /// controlled by the returned AsyncEventSubscriberScope.
  template <class Class>
  AsyncEventSubscriberScope AddListener(Class* obj, std::string_view name,
                                        void (Class::*func)(Args...)) {
    return AddListener(FunctionId(obj), name,
                       [obj, func](Args... args) { (obj->*func)(args...); });
  }

  /// @overload
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
