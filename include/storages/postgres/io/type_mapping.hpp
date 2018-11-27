#pragma once

#include <string>
#include <type_traits>

#include <storages/postgres/detail/is_decl_complete.hpp>
#include <storages/postgres/io/pg_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_traits.hpp>
#include <utils/demangle.hpp>

namespace storages::postgres {

namespace io {

/// @brief Primary template for C++ to Postgres mapping
/// Must provide a Oid GetOid() static member function to get a mapped
/// PostgreSQL type Oid
/// @todo GetOid must accept a structure for querying user-defined type oids
///       per connection.
template <typename T, typename Enable = ::utils::void_t<>>
struct CppToPg;

template <PredefinedOids oid, typename T>
struct PgToCpp;

/// Find out if a text parser for a predefined Postgres type was registered.
bool HasTextParser(PredefinedOids);
/// Find out if a binary parser for a predefined Postgres type was registered.
bool HasBinaryParser(PredefinedOids);

bool HasTextParser(DBTypeName);
bool HasBinaryParser(DBTypeName);

namespace detail {

/// Works as boost::ignore_unused, but returning a value.
template <typename... T>
inline bool ForceReference(const T&...) {
  return true;
}

struct RegisterPredefinedOidParser {
  static RegisterPredefinedOidParser Register(PredefinedOids type_oid,
                                              const std::string& cpp_name,
                                              bool text_parser,
                                              bool bin_parser);
};

// note: implementation is in user_types.cpp
struct RegisterUserTypeParser {
  static RegisterUserTypeParser Register(const DBTypeName&,
                                         const std::string& cpp_name,
                                         bool text_parser, bool bin_parser);
};

/// @brief Declare a mapping of a C++ type to a PostgreSQL type oid.
/// Also mark available parsers
template <typename T>
struct CppToSystemPgImpl {
  using Mapping = CppToSystemPg<T>;
  static constexpr PredefinedOids value = Mapping::value;
  static const inline RegisterPredefinedOidParser init_ =
      RegisterPredefinedOidParser::Register(
          value, ::utils::GetTypeName(std::type_index{typeid(T)}),
          io::traits::kHasTextParser<T>, io::traits::kHasBinaryParser<T>);

  static constexpr Oid GetOid(const UserTypes&) {
    ForceReference(init_);
    return static_cast<Oid>(value);
  }
};

template <typename T>
struct CppToUserPgImpl;

/// @brief Declare a mapping of a PostgreSQL predefined type oid to a C++ type.
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

template <typename T>
struct CppToPg<T, typename std::enable_if<!traits::kIsSpecialMapping<T> &&
                                          traits::kIsMappedToPg<T>>::type>
    : std::conditional<traits::kIsMappedToSystemType<T>,
                       detail::CppToSystemPgImpl<T>,
                       detail::CppToUserPgImpl<T>>::type {};

}  // namespace io
}  // namespace storages::postgres
