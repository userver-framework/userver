#pragma once

#include <unordered_map>
#include <unordered_set>

#include <storages/postgres/io/pg_types.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages::postgres {

/// @page psql_user_types Mapping a C++ type to PostgreSQL user type.
///
/// In PosgtgreSQL there are four kinds of user types available:
///   - composite (row) types, @see pg_composite_types
///   - enumerations @see pg_enum
///   - ranges @see pg_range
///   - domains
///
/// Domains are essentially some data types with database constraints applied
/// to them, they map to their base data types' C++ counterparts.
///
/// Other user types can be mapped to C++ types, more information on defining
/// a mapped C++ type can be found on respective pages. After a C++ type is
/// defined, it must be mapped to it's PostgreSQL counterpart by specialising
/// CppToUserPg template for the type. C++ types are mapped to PostgreSQL types
/// by their names, so the specialisation for CppToUserPg template must have a
/// `static constexpr` member of type DBTypeName named `postgres_name`.
///
/// @code
/// namespace my_ns {
/// struct MyDataStructure {
///   std::string name;
/// };
/// }
/// namespace storages::postgres::io {
/// template <>
/// struct CppToUserPg<my_ns::MyDataStructure> {
///   static constexpr DBTypeName postgres_name = "my_schema.my_type";
/// }
/// @endcode
///
/// A connection gets the data types' definitions after connect and uses the
/// definitions to map C++ types to PostgreSQL type oids.
///

/// @brief Container for connection-specific user data types.
class UserTypes {
 public:
  Oid FindOid(DBTypeName) const;
  Oid FindArrayOid(DBTypeName) const;
  DBTypeName FindName(Oid) const;
  /// Find name of the base type for a domain or element type for an array.
  /// For the rest of types returns the name for the oid if found.
  DBTypeName FindBaseName(Oid) const;
  /// Find base oid for a domain or element type for an array.
  /// For the rest of types returns the name for the oid if found.
  Oid FindBaseOid(Oid) const;
  Oid FindBaseOid(DBTypeName) const;

  bool HasBinaryParser(Oid) const;
  bool HasTextParser(Oid) const;

  void Reset();
  void AddType(DBTypeDescription&& desc);

 private:
  using DescriptionSet =
      std::unordered_set<DBTypeDescription, DBTypeDescription::NameHash,
                         DBTypeDescription::NamesEqual>;
  using DescriptionIterator = DescriptionSet::const_iterator;
  using MapByOid = std::unordered_map<Oid, DescriptionIterator>;
  using MapByName = std::unordered_map<DBTypeName, DescriptionIterator>;

  DescriptionSet types_;
  MapByOid by_oid_;
  MapByName by_name_;
};

namespace io::detail {

template <typename T>
constexpr DBTypeName kPgUserTypeName = CppToUserPg<T>::postgres_name;

template <typename T>
struct CppToUserPgImpl {
  using Mapping = CppToUserPg<T>;
  static constexpr DBTypeName postgres_name = kPgUserTypeName<T>;
  static const detail::RegisterUserTypeParser init_;
  static Oid GetOid(const UserTypes& user_types) {
    ForceReference(init_);
    // TODO Handle oid not found
    return user_types.FindOid(postgres_name);
  }
  static Oid GetArrayOid(const UserTypes& user_types) {
    ForceReference(init_);
    // TODO Handle oid not found
    return user_types.FindArrayOid(postgres_name);
  }
};

template <typename T>
const RegisterUserTypeParser CppToUserPgImpl<T>::init_ =
    RegisterUserTypeParser::Register(kPgUserTypeName<T>,
                                     ::utils::GetTypeName<T>(),
                                     io::traits::kHasTextParser<T>,
                                     io::traits::kHasBinaryParser<T>);

}  // namespace io::detail

}  // namespace storages::postgres
