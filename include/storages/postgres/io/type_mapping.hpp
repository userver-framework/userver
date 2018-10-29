#pragma once

#include <storages/postgres/detail/is_decl_complete.hpp>
#include <storages/postgres/io/pg_types.hpp>

#include <type_traits>

namespace storages {
namespace postgres {

namespace io {

/// @brief Primary template for C++ to Postgres mapping
/// Must provide a Oid GetOid() static member function to get a mapped
/// PostgreSQL type Oid
/// @todo GetOid must accept a structure for querying user-defined type oids
///       per connection (or per instance?).
template <typename T>
struct CppToPg;

namespace traits {

template <typename T>
struct HasPgMapping : utils::IsDeclComplete<CppToPg<T>> {};

}  // namespace traits

namespace detail {

template <PredefinedOids TypeOid>
struct CppToPgPredefined : std::integral_constant<PredefinedOids, TypeOid> {
  using BaseType = std::integral_constant<PredefinedOids, TypeOid>;
  static constexpr Oid GetOid() { return static_cast<Oid>(BaseType::value); }
};

}  // namespace detail

}  // namespace io
}  // namespace postgres
}  // namespace storages
