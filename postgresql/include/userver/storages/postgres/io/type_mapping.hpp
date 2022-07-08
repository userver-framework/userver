#pragma once

#include <string>
#include <type_traits>

#include <userver/compiler/demangle.hpp>
#include <userver/storages/postgres/detail/is_decl_complete.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

/// @brief Detect mapping of a C++ type to Postgres type.
template <typename T, typename Enable = USERVER_NAMESPACE::utils::void_t<>>
struct CppToPg;

template <PredefinedOids oid, typename T>
struct PgToCpp;

/// Find out if a parser for a predefined Postgres type was registered.
bool HasParser(PredefinedOids);
/// Find out if predefined Postgres types are mapped to the same cpp type
bool MappedToSameType(PredefinedOids, PredefinedOids);
/// Get array element oid for a predefined type
PredefinedOids GetArrayElementOid(PredefinedOids);
/// Get the buffer category for a predefined type
BufferCategory GetBufferCategory(PredefinedOids);

bool HasParser(DBTypeName);

namespace detail {

/// Works as boost::ignore_unused, but returning a value.
template <typename... T>
inline bool ForceReference(const T&...) {
  return true;
}

struct RegisterPredefinedOidParser {
  static RegisterPredefinedOidParser Register(PredefinedOids type_oid,
                                              PredefinedOids array_oid,
                                              BufferCategory category,
                                              std::string cpp_name);
};

// note: implementation is in user_types.cpp
struct RegisterUserTypeParser {
  static RegisterUserTypeParser Register(const DBTypeName&,
                                         std::string cpp_name);
  const DBTypeName postgres_name;
};

/// @brief Declare a mapping of a C++ type to a PostgreSQL type oid.
/// Also mark available parsers
template <typename T>
struct CppToSystemPgImpl {
  using Mapping = CppToSystemPg<T>;
  static constexpr auto type_oid = Mapping::value;
  static_assert(type_oid != PredefinedOids::kInvalid, "Type oid is invalid");

  using ArrayMapping = ArrayType<type_oid>;
  static constexpr auto array_oid = ArrayMapping::value;
  static_assert(array_oid != PredefinedOids::kInvalid,
                "Array type oid is invalid");

  // We cannot perform a static check for parser presence here as there are
  // types that should not have one, e.g. char[N]. All parser checks will be
  // performed at runtime.

  static const inline RegisterPredefinedOidParser init_ =
      RegisterPredefinedOidParser::Register(type_oid, array_oid,
                                            io::traits::kTypeBufferCategory<T>,
                                            compiler::GetTypeName<T>());

  static constexpr Oid GetOid(const UserTypes&) {
    ForceReference(init_);
    return static_cast<Oid>(type_oid);
  }
  static constexpr Oid GetArrayOid(const UserTypes&) {
    ForceReference(init_);
    return static_cast<Oid>(array_oid);
  }
};

template <typename T>
struct CppToUserPgImpl;

/// @brief Declare a mapping of a PostgreSQL predefined type oid to a C++ type.
/// Available parsers will be marked after referencing the init_
/// somewhere in the code.
template <PredefinedOids TypeOid, typename T>
struct PgToCppPredefined {
  static constexpr auto type_oid = TypeOid;
  static_assert(type_oid != PredefinedOids::kInvalid, "Type oid is invalid");

  using ArrayMapping = ArrayType<type_oid>;
  static constexpr auto array_oid = ArrayMapping::value;
  static_assert(array_oid != PredefinedOids::kInvalid,
                "Array type oid is invalid");

  // See above on parser presence checks.

  static const inline RegisterPredefinedOidParser init_ =
      RegisterPredefinedOidParser::Register(type_oid, array_oid,
                                            io::traits::kTypeBufferCategory<T>,
                                            compiler::GetTypeName<T>());
};

}  // namespace detail

template <typename T>
struct CppToPg<T, std::enable_if_t<!traits::kIsSpecialMapping<T> &&
                                   traits::kIsMappedToPg<T> &&
                                   !traits::kIsMappedToArray<T>>>
    : std::conditional_t<traits::kIsMappedToSystemType<T>,
                         detail::CppToSystemPgImpl<T>,
                         detail::CppToUserPgImpl<T>> {};

void LogRegisteredTypes();

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
