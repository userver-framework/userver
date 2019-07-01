#pragma once

#include <algorithm>
#include <chrono>
#include <shared_mutex>

#include <engine/mutex.hpp>
#include <logging/log.hpp>
#include <utils/async.hpp>

namespace utils {

namespace impl {
extern const std::chrono::seconds kSubscriberErrorTimeout;
}

class AsyncEventChannelBase {
 public:
  using FunctionId = intptr_t;

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
    scope.id_ = 0;
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
      id_ = 0;
    }
  }

  ~AsyncEventSubscriberScope() {
    if (id_) {
      LOG_ERROR() << "Event subscriber has't unregistered itself, you have "
                     "to store AsyncEventSubscriberScope and explicitly call "
                     "Unsubscribe() when subscription is not needed.";
      UASSERT_MSG(false,
                  "Unsubscribe() is not called for AsyncEventSubscriberScope");
      channel_->RemoveListener(id_);
    }
  }

 private:
  AsyncEventChannelBase* channel_{nullptr};
  FunctionId id_{0};
};

/* AsyncEventChannel is a in-process pubsub with strict FIFO serialization. */
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
    if (it != listeners_.end()) throw std::runtime_error("already subscribed");

    listeners_.emplace_back(id, std::move(name), std::move(func));
    return AsyncEventSubscriberScope(this, id);
  }

  template <class Class>
  [[nodiscard]] AsyncEventSubscriberScope AddListener(
      Class* obj, std::string name, void (Class::*func)(Args...)) {
    return AddListener(reinterpret_cast<FunctionId>(obj), std::move(name),
                       [obj, func](Args... args) { (obj->*func)(args...); });
  }

  template <class Class>
  [[nodiscard]] AsyncEventSubscriberScope AddListener(
      const Class* obj, std::string name, void (Class::*func)(Args...) const) {
    return AddListener(reinterpret_cast<FunctionId>(obj), std::move(name),
                       [obj, func](Args... args) { (obj->*func)(args...); });
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

  struct Listener {
    Listener(FunctionId id, std::string name, Function function)
        : id(id), name(std::move(name)), function(std::move(function)) {}

    FunctionId id;
    std::string name;
    Function function;
  };

  typename std::vector<Listener>::iterator FindListener(FunctionId id) {
    return std::find_if(
        listeners_.begin(), listeners_.end(),
        [id](const Listener& listener) { return listener.id == id; });
  }

  const std::string name_;
  mutable engine::Mutex mutex_;
  std::vector<Listener> listeners_;
};

}  // namespace utils
