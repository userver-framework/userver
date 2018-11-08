#pragma once

#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages {
namespace postgres {

/// @brief Type to represent a null value
template <typename T>
struct Null {};

/// @brief Template variable to use with query parameters.
/// @code
/// trx->Execute("update a_table set val = $1 where val = $2", null<int>, 0)
/// @endcode
template <typename T>
constexpr Null<T> null{};

namespace io {
namespace traits {

template <typename T>
struct IsNullable<Null<T>> : std::true_type {};

template <typename T>
struct GetSetNull<Null<T>> {
  inline static bool IsNull(const Null<T>&) { return true; }
  static void SetNull(T&) {}
};

}  // namespace traits

template <typename T>
struct CppToPg<Null<T>> : CppToPg<T> {};

template <typename T>
struct BufferFormatter<Null<T>, DataFormat::kBinaryDataFormat> {
  explicit BufferFormatter(const Null<T>&) {}

  template <typename Buffer>
  void operator()(Buffer&) const {}
};

}  // namespace io

}  // namespace postgres
}  // namespace storages
