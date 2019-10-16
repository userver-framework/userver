#pragma once

#include <exception>
#include <functional>

#include <logging/log.hpp>
#include <utils/assert.hpp>

namespace utils {

/**
 * @brief a helper class to perform actions on scope exit
 *
 * @note exception handling is done in such way so std::terminate
 *  will not be called: in normal path of executon, exception from handler will
 *  propagate into client code, but if we leave scope because of an exception,
 *  handler's exception, if thrown, will be silenced and written into log
 *  to avoid std::terminate
 */
class ScopeGuard {
 public:
  using Callback = std::function<void()>;
  explicit ScopeGuard(Callback callback) : callback_(std::move(callback)) {}

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&) = delete;

  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  ~ScopeGuard() noexcept(false) {
    if (!callback_) return;

    // FIXME: use std::uncaught_exceptions() here and in ctor
    // when move to C++17 lib to support usecase inside dtor that is called
    // during stack unwinding
    if (std::uncaught_exception()) {
      /* keep all exceptions inside destructor to avoid std::terminate */
      try {
        callback_();
      } catch (const std::exception& e) {
        UASSERT_MSG(false, "exception is thrown during stack unwinding");
        LOG_ERROR() << "exception is thrown during stack unwinding - ignoring: "
                    << e;
      }
    } else {
      /* safe to throw */
      callback_();
    }
  }

  void Release() { callback_ = {}; }

 private:
  Callback callback_;
};

}  // namespace utils
