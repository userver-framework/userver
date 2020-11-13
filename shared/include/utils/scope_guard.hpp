#pragma once

/// @file utils/scope_guard.hpp
/// @brief @copybrief utils::ScopeGuard

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
class ScopeGuard final {
 public:
  using Callback = std::function<void()>;
  explicit ScopeGuard(Callback callback)
      : callback_(std::move(callback)),
        exceptions_on_enter_(std::uncaught_exceptions()) {}

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&) = delete;

  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  ~ScopeGuard() noexcept(false) {
    if (!callback_) return;

    if (std::uncaught_exceptions() != exceptions_on_enter_) {
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
  const int exceptions_on_enter_;
};

}  // namespace utils
