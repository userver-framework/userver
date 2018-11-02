#pragma once

#include <algorithm>
#include <shared_mutex>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace utils {

class AsyncEventChannelBase {
 public:
  using FunctionId = intptr_t;

 protected:
  friend class AsyncEventSubscriberScope;

  virtual bool RemoveListener(FunctionId id) noexcept = 0;
};

class AsyncEventSubscriberScope {
 public:
  using FunctionId = AsyncEventChannelBase::FunctionId;

  AsyncEventSubscriberScope() : channel_(nullptr), id_(0) {}

  AsyncEventSubscriberScope(AsyncEventChannelBase* channel, FunctionId id)
      : channel_(channel), id_(id) {}

  AsyncEventSubscriberScope(AsyncEventSubscriberScope&& scope)
      : channel_(scope.channel_), id_(scope.id_) {
    scope.id_ = 0;
  }

  AsyncEventSubscriberScope& operator=(AsyncEventSubscriberScope&& other) {
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
      assert(false &&
             "Unsubscribe() is not called for AsyncEventSubscriberScope");
      channel_->RemoveListener(id_);
    }
  }

 private:
  AsyncEventChannelBase* channel_;
  FunctionId id_;
};

template <typename... Args>
class AsyncEventChannel : public AsyncEventChannelBase {
 public:
  using Function = std::function<void(Args... args)>;

  AsyncEventSubscriberScope AddListener(FunctionId id, Function func) {
    std::lock_guard<std::shared_timed_mutex> lock(mutex_);

    auto it = FindListener(id);
    if (it != listeners_.end()) throw std::runtime_error("already subscribed");

    listeners_.emplace_back(id, std::move(func));
    return AsyncEventSubscriberScope(this, id);
  }

  template <class Class>
  AsyncEventSubscriberScope AddListener(Class* obj,
                                        void (Class::*func)(Args...)) {
    return AddListener(reinterpret_cast<FunctionId>(obj),
                       [obj, func](Args... args) { (obj->*func)(args...); });
  }

  template <class Class>
  AsyncEventSubscriberScope AddListener(const Class* obj,
                                        void (Class::*func)(Args...) const) {
    return AddListener(reinterpret_cast<FunctionId>(obj),
                       [obj, func](Args... args) { (obj->*func)(args...); });
  }

  void SendEvent(const Args&... args) const {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    for (const Listener& listener : listeners_) {
      engine::CriticalAsync(listener.second, args...).Detach();
    }
  }

 private:
  bool RemoveListener(FunctionId id) noexcept override {
    std::lock_guard<std::shared_timed_mutex> lock(mutex_);

    auto it = FindListener(id);
    if (it == listeners_.end()) {
      LOG_ERROR()
          << logging::LogExtra::Stacktrace()
          << "Trying to unregister a subscriber which is not registered";
      assert(false);
      return false;
    }

    listeners_.erase(it);
    return true;
  }

  using Listener = std::pair<FunctionId, Function>;

  typename std::vector<Listener>::iterator FindListener(FunctionId id) {
    return std::find_if(
        listeners_.begin(), listeners_.end(),
        [id](const Listener& listener) { return listener.first == id; });
  }

  mutable std::shared_timed_mutex mutex_;
  std::vector<Listener> listeners_;
};

}  // namespace utils
