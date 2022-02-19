#pragma once

#include <type_traits>
#include <utility>

/// @file userver/utils/fast_scope_guard.hpp
/// @brief @copybrief utils::FastScopeGuard

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief a helper class to perform actions on scope exit
///
/// The lambda argument must be explicitly marked as `noexcept`. Only use
/// FastScopeGuard if it's proven that the lambda body is `noexcept`, otherwise
/// use ScopeGuard.
///
/// Usage example:
/// @sample shared/src/utils/fast_scope_guard_test.cpp  FastScopeGuard example
///
/// @see ScopeGuard for type-erasure and throwing functor support
template <typename Callback>
class FastScopeGuard final {
 public:
  static_assert(std::is_nothrow_move_constructible_v<Callback>);

  static_assert(
      std::is_nothrow_invocable_r_v<void, Callback&&>,
      "If the functions called in the body of the lambda are all 'noexcept', "
      "please mark the lambda itself as 'noexcept'. If however, the contents "
      "are not 'noexcept', use 'ScopeGuard' instead of 'FastScopeGuard'.");

  constexpr explicit FastScopeGuard(Callback callback) noexcept
      : callback_(std::move(callback)) {}

  constexpr FastScopeGuard(FastScopeGuard&& other) noexcept
      : callback_(std::move(other.callback_)),
        is_active_(std::exchange(other.is_active_, false)) {}

  constexpr FastScopeGuard& operator=(FastScopeGuard&& other) noexcept {
    if (this != &other) {
      callback_ = std::move(other.callback_);
      is_active_ = std::exchange(other.is_active_, false);
    }
    return *this;
  }

  ~FastScopeGuard() {
    if (is_active_) std::move(callback_)();
  }

  constexpr void Release() noexcept { is_active_ = false; }

 private:
  Callback callback_;

  // should be optimized out if 'Release' is not called
  bool is_active_{true};
};

}  // namespace utils

USERVER_NAMESPACE_END
