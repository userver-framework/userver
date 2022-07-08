#pragma once

#include <type_traits>

#include <userver/compiler/demangle.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::traits {

/// @brief Metafunction to detect nullability of a type.
template <typename T>
struct IsNullable : std::false_type {};
template <typename T>
inline constexpr bool kIsNullable = IsNullable<T>::value;

template <typename T>
struct GetSetNull {
  inline static bool IsNull(const T&) { return false; }
  inline static void SetNull(T&) {
    // TODO Consider a static_assert here
    throw TypeCannotBeNull(compiler::GetTypeName<T>());
  }
  inline static void SetDefault(T& value) { value = T{}; }
};

}  // namespace storages::postgres::io::traits

USERVER_NAMESPACE_END
