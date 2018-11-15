#pragma once

#include <string>
#include <type_traits>

#include <storages/postgres/detail/is_decl_complete.hpp>
#include <storages/postgres/io/pg_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <utils/demangle.hpp>

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

template <PredefinedOids oid, typename T>
struct PgToCpp;

namespace traits {

template <typename T>
struct HasPgMapping : utils::IsDeclComplete<CppToPg<T>> {};

template <typename T>
constexpr bool kHasPgMapping = HasPgMapping<T>::value;

}  // namespace traits

/// Find out if a text parser for a predefined Postgres type was registered.
bool HasTextParser(PredefinedOids);
/// Find out if a binary parser for a predefined Postgres type was registered.
bool HasBinaryParser(PredefinedOids);

namespace detail {

/// Works as boost::ignore_unused, but returning a value.
template <typename... T>
inline bool ForceReference(const T&...) {
  return true;
}

struct RegisterPredefinedOidParser {
  RegisterPredefinedOidParser() {}

  static RegisterPredefinedOidParser Register(PredefinedOids type_oid,
                                              const std::string& cpp_name,
                                              bool text_parser,
                                              bool bin_parser);
};

/// @brief Declare a mapping of a C++ type to a PostgreSQL type oid.
/// Also mark available parsers
template <typename T, PredefinedOids TypeOid>
struct CppToPgPredefined : std::integral_constant<PredefinedOids, TypeOid> {
  using BaseType = std::integral_constant<PredefinedOids, TypeOid>;
  static const inline RegisterPredefinedOidParser init_ =
      RegisterPredefinedOidParser::Register(
          TypeOid, ::utils::GetTypeName(std::type_index{typeid(T)}),
          io::traits::kHasTextParser<T>, io::traits::kHasBinaryParser<T>);

  static constexpr Oid GetOid() {
    ForceReference(init_);
    return static_cast<Oid>(BaseType::value);
  }
};

/// @brief Declare a mapping of a PostgreSQL type oid to a C++ type.
/// Available parsers will be marked after referencing the init_
/// somewhere in the code.
template <PredefinedOids TypeOid, typename T>
struct PgToCppPredefined {
  static const inline RegisterPredefinedOidParser init_ =
      RegisterPredefinedOidParser::Register(
          TypeOid, ::utils::GetTypeName(std::type_index{typeid(T)}),
          io::traits::kHasTextParser<T>, io::traits::kHasBinaryParser<T>);
};

}  // namespace detail

}  // namespace io
}  // namespace postgres
}  // namespace storages
