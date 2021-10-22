#pragma once

#include <type_traits>

#include <userver/compiler/demangle.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace postgres {
namespace io {
namespace traits {

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

}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages

USERVER_NAMESPACE_END
