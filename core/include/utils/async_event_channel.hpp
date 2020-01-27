#pragma once

#include <chrono>
#include <utility>

#include <engine/mutex.hpp>
#include <logging/log.hpp>
#include <utils/async.hpp>

namespace utils {

namespace impl {
extern const std::chrono::seconds kSubscriberErrorTimeout;
}

class AsyncEventChannelBase {
 public:
  class FunctionId final {
   public:
    template <typename Class>
    explicit FunctionId(Class* obj) : ptr_(obj), type_index_(typeid(Class)) {}

    FunctionId() : ptr_(nullptr), type_index_(typeid(void)) {}

    explicit operator bool() const { return ptr_ != 0; }

    bool operator==(const FunctionId& other) const {
      return ptr_ == other.ptr_ && type_index_ == other.type_index_;
    }

   private:
    void* ptr_;
    std::type_index type_index_;
  };

 protected:
  friend class AsyncEventSubscriberScope;

  virtual bool RemoveListener(FunctionId id) noexcept = 0;
};

class AsyncEventSubscriberScope final {
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

/// AsyncEventChannel is a in-process pubsub with strict FIFO serialization.
template <typename... Args>
class AsyncEventChannel : public AsyncEventChannelBase {
 public:
  using Function = std::function<void(Args... args)>;

  explicit AsyncEventChannel(std::string name) : name_(std::move(name)) {}

  [[nodiscard]] AsyncEventSubscriberScope AddListener(FunctionId id,
                                                      std::string name,
                                                      Function func) {
    std::lock_guard<engine::Mutex> lock(mutex_);

    auto it = FindListener(id);
    if (it != listeners_.end()) {
      UASSERT(false);
      throw std::runtime_error("already subscribed");
    }

    listeners_.emplace_back(id, std::move(name), std::move(func));
    return AsyncEventSubscriberScope(this, id);
  }

  template <class Class>
  [[nodiscard]] AsyncEventSubscriberScope AddListener(
      Class* obj, std::string name, void (Class::*func)(Args...)) {
    return AddListener(FunctionId(obj), std::move(name),
                       [obj, func](Args... args) {
                         (obj->*func)(std::forward<Args>(args)...);
                       });
  }

  template <class Class>
  [[nodiscard]] AsyncEventSubscriberScope AddListener(
      const Class* obj, std::string name, void (Class::*func)(Args...) const) {
    return AddListener(FunctionId(obj), std::move(name),
                       [obj, func](Args... args) {
                         (obj->*func)(std::forward<Args>(args)...);
                       });
  }

  void SendEvent(const Args&... args) const {
    std::lock_guard<engine::Mutex> lock(mutex_);

    std::vector<std::pair<std::string, engine::TaskWithResult<void>>>
        subscribers;
    for (const Listener& listener : listeners_) {
      const auto& subscriber_name = listener.name;
      subscribers.push_back(std::make_pair(
          subscriber_name,
          utils::Async("async_channel/" + name_ + '_' + subscriber_name,
                       listener.function, args...)));
    }

    // Wait for all tasks regardless of the result
    for (auto& subscriber : subscribers) {
      WaitForTask(subscriber.first, subscriber.second);
    }

    for (auto& task : subscribers) {
      try {
        task.second.Get();
      } catch (const std::exception& e) {
        LOG_ERROR() << "Unhandled exception in subscriber " << task.first
                    << ": " << e;
      }
    }
  }

 private:
  bool RemoveListener(FunctionId id) noexcept override {
    std::lock_guard<engine::Mutex> lock(mutex_);

    auto it = FindListener(id);
    if (it == listeners_.end()) {
      LOG_ERROR()
          << logging::LogExtra::Stacktrace()
          << "Trying to unregister a subscriber which is not registered";
      UASSERT(false);
      return false;
    }

    listeners_.erase(it);
    return true;
  }

  void WaitForTask(const std::string& name,
                   const engine::TaskWithResult<void>& task) const {
    while (true) {
      task.WaitFor(impl::kSubscriberErrorTimeout);
      if (task.IsFinished()) return;

      LOG_ERROR() << "Subscriber " << name << " handles event for too long";
    }
  }

  struct Listener final {
    Listener(FunctionId id, std::string name, Function function)
        : id(id), name(std::move(name)), function(std::move(function)) {}

    FunctionId id;
    std::string name;
    Function function;
  };

  typename std::vector<Listener>::iterator FindListener(FunctionId id) {
    // not using std::find_if to avoid incusion of <algorithm> in a header file
    const auto end = listeners_.end();
    for (auto it = listeners_.begin(); it != end; ++it) {
      if (it->id == id) return it;
    }
    return end;
  }

  const std::string name_;
  mutable engine::Mutex mutex_;
  std::vector<Listener> listeners_;
};

}  // namespace utils
