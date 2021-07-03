#pragma once

#include <type_traits>

#include <compiler/demangle.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace io {
namespace traits {

/// @brief Metafunction to detect nullability of a type.
template <typename T>
struct IsNullable : std::false_type {};
template <typename T>
constexpr bool kIsNullable = IsNullable<T>::value;

template <typename T>
struct GetSetNull {
  inline static bool IsNull(const T&) { return false; }
  inline static void SetNull(T&) {
    // TODO Consider a static_assert here
    throw TypeCannotBeNull(::compiler::GetTypeName<T>());
  }
  inline static void SetDefault(T& value) { value = T{}; }
};

}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages
