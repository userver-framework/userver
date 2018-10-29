#pragma once

#include <type_traits>

namespace storages {
namespace postgres {
namespace io {
namespace traits {

/// @brief Metafunction to detect nullability of a type.
template <typename T>
struct IsNullable : std::false_type {};

template <typename T>
struct GetSetNull {
  inline static bool IsNull(const T&) { return false; }
  static void SetNull(T&) {
    // TODO Throw an error here
  }
};

}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages
