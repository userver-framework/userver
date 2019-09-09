#pragma once

#include <functional>

namespace utils {

class ScopeGuard {
 public:
  using Callback = std::function<void()>;
  explicit ScopeGuard(Callback callback) : callback_(std::move(callback)) {}

  ~ScopeGuard() noexcept(false) {
    if (callback_) callback_();
  }

  void Release() { callback_ = {}; }

 private:
  Callback callback_;
};

}  // namespace utils
