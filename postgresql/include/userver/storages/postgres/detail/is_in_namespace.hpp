#pragma once

#include <string_view>
#include <type_traits>

#include <boost/current_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

#ifdef __clang__
constexpr std::string_view kExpectedPrefix =
    "postgres::detail::IsInNamespaceImpl(std::string_view) [T = ";
#else
constexpr std::string_view kExpectedPrefix =
    "postgres::detail::IsInNamespaceImpl(std::string_view) [with T = ";
#endif

constexpr bool StartsWith(std::string_view haystack, std::string_view needle) {
  return haystack.substr(0, needle.size()) == needle;
}

template <typename T>
constexpr bool IsInNamespaceImpl(std::string_view nsp) {
  constexpr std::string_view fname = BOOST_CURRENT_FUNCTION;
  constexpr auto pos = fname.find(kExpectedPrefix);
  if (pos == std::string_view::npos) {
    return false;
  }
  constexpr std::string_view fname_short{fname.data() + pos,
                                         fname.size() - pos};
  static_assert(!fname_short.empty(),
                "Your compiler produces an unexpected function pretty name");
  return StartsWith(fname_short.substr(kExpectedPrefix.size()), nsp) &&
         StartsWith(fname_short.substr(kExpectedPrefix.size() + nsp.size()),
                    "::");
}

template <typename T>
constexpr bool IsInNamespace(std::string_view nsp) {
  return IsInNamespaceImpl<std::remove_const_t<std::decay_t<T>>>(nsp);
}

template <typename T>
inline constexpr bool kIsInStdNamespace = IsInNamespace<T>("std");
template <typename T>
inline constexpr bool kIsInBoostNamespace = IsInNamespace<T>("boost");

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
