#pragma once

#include <functional>
#include <mutex>

namespace engine {

class Notifier {
 public:
  using NotifyCb = std::function<void()>;

  Notifier(NotifyCb notify_cb)
      : notify_cb_(std::move(notify_cb)), is_enabled_(true) {}
  virtual ~Notifier() { Disable(); }

  void Notify() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_enabled_) {
      notify_cb_();
    }
  }

  void Disable() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    is_enabled_ = false;
  }

 private:
  NotifyCb notify_cb_;

  std::mutex mutex_;
  bool is_enabled_;
};

}  // namespace engine
