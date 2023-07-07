#pragma once

/// @file userver/utils/void_t.hpp
/// @brief @copybrief utils::void_t

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

#if (__cpp_lib_void_t >= 201411 && !defined(__clang__)) || defined(DOXYGEN)
/// @brief std::void_t implementation with workarounds for compiler bugs
template <typename... T>
using void_t = std::void_t<T...>;
#else

template <typename... T>
struct make_void_t {
  using type = void;
};
template <typename... T>
using void_t = typename make_void_t<T...>::type;

#endif

}  // namespace utils

USERVER_NAMESPACE_END
