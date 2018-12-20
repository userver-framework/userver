#pragma once

#include <boost/current_function.hpp>
#include <type_traits>

#include <utils/strlen.hpp>

namespace storages::postgres::detail {

// This signature is produced with clang. When we will support other
// compilers, #ifdefs will be required
constexpr const char* kExpectedSignature =
    "bool storages::postgres::detail::IsInNamespaceImpl(const char *) [T = ";

constexpr std::size_t kTypeNameStart = ::utils::StrLen(kExpectedSignature);

constexpr bool StartsWith(const char* str, const char* expected) {
  for (; *str != '\0' && *expected != '\0' && *str == *expected;
       ++str, ++expected) {
  }
  return *expected == '\0';
}

template <typename T>
constexpr bool IsInNamespaceImpl(const char* nsp) {
  constexpr const char* fname = BOOST_CURRENT_FUNCTION;
  static_assert(StartsWith(fname, kExpectedSignature),
                "Your compiler produces an unexpected function pretty name");
  auto c = fname + kTypeNameStart;
  return StartsWith(fname + kTypeNameStart, nsp) &&
         StartsWith(fname + kTypeNameStart + ::utils::StrLen(nsp), "::");
}

template <typename T>
constexpr bool IsInNamespace(const char* nsp) {
  return IsInNamespaceImpl<std::remove_const_t<std::decay_t<T>>>(nsp);
}

template <typename T>
struct IsInStdNamespace
    : std::integral_constant<bool, IsInNamespace<T>("std")> {};
template <typename T>
struct IsInBoostNamespace
    : std::integral_constant<bool, IsInNamespace<T>("boost")> {};

}  // namespace storages::postgres::detail
