#pragma once

#include <string_view>
#include <type_traits>

#include <boost/current_function.hpp>

namespace storages::postgres::detail {

// This signature is produced with clang. When we will support other
// compilers, #ifdefs will be required
constexpr std::string_view kExpectedSignature =
    "bool storages::postgres::detail::IsInNamespaceImpl(std::string_view) [T "
    "= ";

constexpr bool StartsWith(std::string_view haystack, std::string_view needle) {
  return haystack.substr(0, needle.size()) == needle;
}

template <typename T>
constexpr bool IsInNamespaceImpl(std::string_view nsp) {
  constexpr std::string_view fname = BOOST_CURRENT_FUNCTION;
  static_assert(StartsWith(fname, kExpectedSignature),
                "Your compiler produces an unexpected function pretty name");
  return StartsWith(fname.substr(kExpectedSignature.size()), nsp) &&
         StartsWith(fname.substr(kExpectedSignature.size() + nsp.size()), "::");
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
