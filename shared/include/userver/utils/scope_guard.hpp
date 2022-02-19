#pragma once

/// @file userver/utils/scope_guard.hpp
/// @brief @copybrief utils::ScopeGuard

#include <exception>
#include <functional>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief a helper class to perform actions on scope exit
///
/// Usage example:
/// @sample shared/src/utils/scope_guard_test.cpp  ScopeGuard usage example
///
/// @note exception handling is done in such way that std::terminate will not be
/// called: in the normal path of execution, exception from handler will
/// propagate into client code, but if we leave scope because of an exception,
/// handler's exception, if thrown, will be silenced and written into log to
/// avoid std::terminate
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
      // keep all exceptions inside the destructor to avoid std::terminate
      try {
        callback_();
      } catch (const std::exception& e) {
        UASSERT_MSG(false, "exception is thrown during stack unwinding");
        LOG_ERROR() << "Exception is thrown during stack unwinding - ignoring: "
                    << e;
      }
    } else {
      // safe to throw
      callback_();
    }
  }

  void Release() noexcept { callback_ = {}; }

 private:
  Callback callback_;
  const int exceptions_on_enter_;
};

}  // namespace utils

USERVER_NAMESPACE_END
