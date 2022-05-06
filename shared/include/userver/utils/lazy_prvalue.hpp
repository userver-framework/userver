#pragma once

/// @file userver/utils/lazy_prvalue.hpp
/// @brief @copybrief utils::LazyPrvalue

#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Can be used with various emplace functions to allow in-place
/// constructing a non-movable value using a callable.
///
/// @snippet shared/src/utils/lazy_prvalue_test.cpp  LazyPrvalue sample
template <typename Func>
class LazyPrvalue final {
  static_assert(!std::is_reference_v<Func>);

 public:
  constexpr explicit LazyPrvalue(const Func& func) : func_(func) {}

  constexpr explicit LazyPrvalue(Func&& func) : func_(std::move(func)) {}

  LazyPrvalue(LazyPrvalue&&) = delete;
  LazyPrvalue& operator=(LazyPrvalue&&) = delete;

  constexpr /* implicit */ operator std::invoke_result_t<Func&&>() && {
    return std::move(func_)();
  }

 private:
  Func func_;
};

}  // namespace utils

USERVER_NAMESPACE_END
