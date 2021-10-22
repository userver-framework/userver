#pragma once

/// @file userver/storages/postgres/null.hpp
/// @brief NULL type

#include <userver/storages/postgres/io/nullable_traits.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @brief Type to represent a null value
template <typename T>
struct Null {};

/// @brief Template variable to use with query parameters.
/// @code
/// trx->Execute("update a_table set val = $1 where val = $2", null<int>, 0)
/// @endcode
template <typename T>
inline constexpr Null<T> null{};

namespace io {
namespace traits {

template <typename T>
struct IsNullable<Null<T>> : std::true_type {};

template <typename T>
struct GetSetNull<Null<T>> {
  inline static bool IsNull(const Null<T>&) { return true; }
  static void SetNull(T&) {}
};

template <typename T>
struct IsMappedToPg<Null<T>> : IsMappedToPg<T> {};
template <typename T>
struct IsSpecialMapping<Null<T>> : IsMappedToPg<T> {};

}  // namespace traits

template <typename T>
struct CppToPg<Null<T>> : CppToPg<T> {};

template <typename T>
struct BufferFormatter<Null<T>> {
  explicit BufferFormatter(const Null<T>&) {}

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer&) const {}
};

}  // namespace io

}  // namespace storages::postgres

USERVER_NAMESPACE_END
