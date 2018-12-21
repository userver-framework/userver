#pragma once

#include <unordered_map>
#include <unordered_set>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/pg_types.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages::postgres {
namespace io {
/// @page pg_user_types µPg: Mapping a C++ type to PostgreSQL user type
///
/// In PosgtgreSQL the following kinds of user types are available:
///   - composite (row) types, see @ref pg_composite_types
///   - enumerations, see @ref pg_enum
///   - ranges, not implemented yet
///   - domains
///
/// Domains are essentially some data types with database constraints applied
/// to them, they map to their base data types' C++ counterparts.
///
/// Other user types can be mapped to C++ types, more information on defining
/// a mapped C++ type can be found on respective pages. After a C++ type is
/// defined, it must be mapped to it's PostgreSQL counterpart by specialising
/// CppToUserPg template for the type. C++ types are mapped to PostgreSQL
/// types by their names, so the specialisation for CppToUserPg template
/// must have a `static constexpr` member of type DBTypeName named
/// `postgres_name`.
///
/// @par C++ type
///
/// @snippet storages/postgres/tests/user_types_pg_test.cpp User type
///
/// @par Declaring C++ type to PostgreSQL type mapping
///
/// @warning The type mapping specialisation **must** be accessible at the
/// points where parsing/formatting of the C++ type is instantiated. The
/// header where the C++ type is declared is an appropriate place to do it.
///
/// @snippet storages/postgres/tests/user_types_pg_test.cpp User type mapping
///
/// A connection gets the data types' definitions after connect and uses the
/// definitions to map C++ types to PostgreSQL type oids.
///
}  // namespace io

/// @brief PostgreSQL composite type description
class CompositeTypeDescription {
 public:
  using CompositeFieldDefs = std::vector<CompositeFieldDef>;
  CompositeTypeDescription(CompositeFieldDefs::const_iterator begin,
                           CompositeFieldDefs::const_iterator end)
      : attributes_{begin, end} {}
  std::size_t Size() const { return attributes_.size(); }
  bool Empty() const { return attributes_.empty(); }
  const CompositeFieldDef& operator[](std::size_t index) const {
    if (index >= Size()) {
      throw FieldIndexOutOfBounds{index};
    }
    return attributes_[index];
  }

 private:
  CompositeFieldDefs attributes_;
};

/// @brief Container for connection-specific user data types.
class UserTypes {
 public:
  using CompositeFieldDefs = std::vector<CompositeFieldDef>;

 public:
  Oid FindOid(DBTypeName) const;
  Oid FindArrayOid(DBTypeName) const;
  Oid FindElementOid(Oid) const;
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
  void AddCompositeFields(CompositeFieldDefs&& defs);

  const CompositeTypeDescription& GetCompositeDescription(Oid) const;

 private:
  using DescriptionSet =
      std::unordered_set<DBTypeDescription, DBTypeDescription::NameHash,
                         DBTypeDescription::NamesEqual>;
  using DescriptionIterator = DescriptionSet::const_iterator;
  using MapByOid = std::unordered_map<Oid, DescriptionIterator>;
  using MapByName = std::unordered_map<DBTypeName, DescriptionIterator>;
  using CompositeTypes = std::unordered_map<Oid, CompositeTypeDescription>;

  DescriptionSet types_;
  MapByOid by_oid_;
  MapByName by_name_;
  CompositeTypes composite_types_;
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
