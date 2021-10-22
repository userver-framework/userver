#pragma once

#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/clang_format_workarounds.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {
void WaitForTask(const std::string& name, engine::TaskWithResult<void>& task);
}

class AsyncEventChannelBase {
 public:
  virtual ~AsyncEventChannelBase() = default;

  class FunctionId final {
   public:
    constexpr FunctionId() = default;

    template <typename Class>
    explicit FunctionId(Class* obj) : ptr_(obj), type_index_(typeid(Class)) {}

    explicit operator bool() const { return ptr_ != nullptr; }

    bool operator==(const FunctionId& other) const {
      return ptr_ == other.ptr_ && type_index_ == other.type_index_;
    }

    struct Hash final {
      std::size_t operator()(FunctionId id) const noexcept {
        return std::hash<void*>{}(id.ptr_);
      }
    };

   private:
    void* ptr_{nullptr};
    std::type_index type_index_{typeid(void)};
  };

 protected:
  friend class AsyncEventSubscriberScope;

  virtual void RemoveListener(FunctionId id) noexcept = 0;
};

class USERVER_NODISCARD AsyncEventSubscriberScope final {
 public:
  using FunctionId = AsyncEventChannelBase::FunctionId;

  AsyncEventSubscriberScope() = default;

  AsyncEventSubscriberScope(AsyncEventChannelBase* channel, FunctionId id)
      : channel_(channel), id_(id) {}

  AsyncEventSubscriberScope(AsyncEventSubscriberScope&& scope) noexcept
      : channel_(scope.channel_), id_(scope.id_) {
    scope.id_ = {};
  }

  AsyncEventSubscriberScope& operator=(
      AsyncEventSubscriberScope&& other) noexcept {
    std::swap(other.channel_, channel_);
    std::swap(other.id_, id_);
    return *this;
  }

  void Unsubscribe() noexcept {
    if (id_) {
      channel_->RemoveListener(id_);
      id_ = {};
    }
  }

  ~AsyncEventSubscriberScope() {
    if (id_) {
      LOG_DEBUG() << "Unsubscribing automatically";
      Unsubscribe();
    }
  }

 private:
  AsyncEventChannelBase* channel_{nullptr};
  FunctionId id_;
};

/// @ingroup userver_concurrency
///
/// AsyncEventChannel is a in-process pubsub with strict FIFO serialization.
template <typename... Args>
class AsyncEventChannel : public AsyncEventChannelBase {
 public:
  using Function = std::function<void(Args... args)>;

  explicit AsyncEventChannel(std::string name) : name_(std::move(name)) {}

  AsyncEventSubscriberScope AddListener(FunctionId id, std::string name,
                                        Function func) {
    auto listeners = listeners_.Lock();
    auto task_name = "async_channel/" + name_ + '_' + name;
    const auto [iterator, success] = listeners->emplace(
        id, Listener{name, std::move(func), std::move(task_name)});
    UINVARIANT(success, name + " is already subscribed to " + name_);
    return AsyncEventSubscriberScope(this, id);
  }

  template <class Class>
  AsyncEventSubscriberScope AddListener(Class* obj, std::string name,
                                        void (Class::*func)(Args...)) {
    return AddListener(FunctionId(obj), std::move(name),
                       [obj, func](Args... args) { (obj->*func)(args...); });
  }

  /// For internal use by specific event channels. Conceptually, `updater`
  /// should invoke `func` with the previously sent event.
  template <typename Class, typename UpdaterFunc>
  AsyncEventSubscriberScope DoUpdateAndListen(Class* obj, std::string name,
                                              void (Class::*func)(Args...),
                                              UpdaterFunc&& updater) {
    std::lock_guard lock(event_mutex_);
    std::forward<UpdaterFunc>(updater)();
    return AddListener(obj, std::move(name), func);
  }

  void SendEvent(Args... args) const {
    std::lock_guard lock(event_mutex_);
    auto listeners = listeners_.Lock();

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(listeners->size());

    for (const auto& [_, listener] : *listeners) {
      tasks.push_back(utils::Async(
          listener.task_name,
          [&, &callback = listener.callback] { callback(args...); }));
    }

    std::size_t i = 0;
    for (const auto& [_, listener] : *listeners) {
      impl::WaitForTask(listener.task_name, tasks[i++]);
    }
  }

  const std::string& Name() const { return name_; }

 private:
  struct Listener final {
    std::string name;
    Function callback;
    std::string task_name;
  };

  void RemoveListener(FunctionId id) noexcept override {
    engine::TaskCancellationBlocker blocker;
    auto listeners = listeners_.Lock();
    const bool success = listeners->erase(id);

    // RemoveListener is called from destructors, don't throw in production
    if (!success) {
      LOG_ERROR()
          << logging::LogExtra::Stacktrace()
          << "Trying to unregister a subscriber which is not registered";
      UASSERT(false);
    }
  }

  const std::string name_;
  concurrent::Variable<
      std::unordered_map<FunctionId, Listener, FunctionId::Hash>>
      listeners_;
  mutable engine::Mutex event_mutex_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
