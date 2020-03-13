#pragma once

#include <type_traits>

namespace utils {

#if __cpp_lib_void_t >= 201411 && !defined(__clang__)
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
