#pragma once

#include <cstdint>
#include <storages/postgres/detail/db_data_type_name.hpp>
#include <string>

#include <utils/string_view.hpp>
#include <utils/strlen.hpp>

namespace storages::postgres {

/// PostgreSQL Oid type.
// Aliased to unsigned int to match the type used in libpq.
using Oid = unsigned int;

//@{
/** @name Type aliases for integral types */
using Smallint = std::int16_t;
using Integer = std::int32_t;
using Bigint = std::int64_t;
//@}

//@{
using Float4 = float;
using Float8 = double;
//@}

/// @brief Identity for a PostgreSQL type name
struct DBTypeName {
  const ::utils::string_view schema;
  const ::utils::string_view name;

  constexpr DBTypeName() : schema{}, name{} {}
  explicit constexpr DBTypeName(
      std::pair<::utils::string_view, ::utils::string_view> n)
      : schema(n.first), name(n.second) {}
  /// Implicit constructor from a string literal, to enable declarations like
  /// @code
  /// DBTypeName my_type = "my_schema.my_type";
  /// @endcode
  /* implicit */ constexpr DBTypeName(const char* name)
      : DBTypeName(utils::ParseDBName(name)) {}
  constexpr DBTypeName(const char* s, const char* n)
      : schema(s, ::utils::StrLen(s)), name(n, ::utils::StrLen(n)) {}

  bool operator==(const DBTypeName& rhs) const {
    return name == rhs.name && schema == rhs.schema;
  }
  bool operator<(const DBTypeName& rhs) const {
    auto schema_cmp = schema.compare(rhs.schema);
    if (schema_cmp < 0) {
      return true;
    } else if (schema_cmp == 0) {
      return name < rhs.name;
    }
    return false;
  }

  bool Empty() const { return name.empty(); }
  std::size_t GetHash() const;
};

/// @brief Description of a PostgreSQL type.
/// The structure is selected from the pg_catalog.pg_type table (not all, only
/// appropriate fields).
/// See https://www.postgresql.org/docs/10/catalog-pg-type.html
struct DBTypeDescription {
  enum class TypeClass : char {
    kBase = 'b',
    kComposite = 'c',
    kDomain = 'd',
    kEnum = 'e',
    kPseudo = 'p',
    kRange = 'r'
  };
  /// @brief PosgtreSQL type category.
  /// See
  /// https://www.postgresql.org/docs/10/catalog-pg-type.html#CATALOG-TYPCATEGORY-TABLE
  enum class TypeCategory : char {
    kInvalid = 0,  //!< Invalid type category
    kArray = 'A',  //!< https://www.postgresql.org/docs/10/arrays.html
    kBoolean =
        'B',  //!< https://www.postgresql.org/docs/10/datatype-boolean.html
    kComposite = 'C',  //!< https://www.postgresql.org/docs/10/rowtypes.html
    kDatetime =
        'D',  //!< https://www.postgresql.org/docs/10/datatype-datetime.html
    kEnumeration =
        'E',  //!< https://www.postgresql.org/docs/10/datatype-enum.html
    kGeometric =
        'G',  //!< https://www.postgresql.org/docs/10/datatype-geometric.html
    kNetwork =
        'I',  //!< https://www.postgresql.org/docs/10/datatype-net-types.html
    kNumeric =
        'N',  //!< https://www.postgresql.org/docs/10/datatype-numeric.html
    kPseudotype =
        'P',       //!< https://www.postgresql.org/docs/10/datatype-pseudo.html
    kRange = 'R',  //!< https://www.postgresql.org/docs/10/rangetypes.html
    kString =
        'S',  //!< https://www.postgresql.org/docs/10/datatype-character.html
    kTimespan =
        'T',      //!< https://www.postgresql.org/docs/10/datatype-datetime.html
    kUser = 'U',  //!< User types, created by fourth form of create type
                  /// statement
                  /// https://www.postgresql.org/docs/10/sql-createtype.html
    kBitstring = 'V',  //!< https://www.postgresql.org/docs/10/datatype-bit.html
    kUnknown = 'X'     //!< Unknown type category
  };

  /// pg_type.oid
  Oid oid;
  /// pg_namespace.nspname
  std::string schema;
  /// pg_type.typname
  std::string name;
  /// pg_type.typlen
  Integer length;
  /// pg_type.typtype
  TypeClass type_class;
  /// pg_type.typcategory
  TypeCategory category;
  /// pg_type.typdelim
  char delim;
  /// pg_type.typrelid
  Oid relation_id;
  /// pg_type.typelem
  /// If not zero, this type is an array
  Oid element_type;
  /// pg_type.typarray
  Oid array_type;
  /// pg_type.typbasetype
  /// Base type for domains
  Oid base_type;
  /// pg_type.typnotnull
  /// Used only with domains
  bool not_null;
  // TODO rest of domain fields. or eliminate them if they are not needed

  DBTypeName GetName() const { return {schema.c_str(), name.c_str()}; }
  std::size_t GetNameHash() const;

  struct NameHash {
    std::size_t operator()(const DBTypeDescription& type) const {
      return type.GetNameHash();
    }
  };
  struct NamesEqual {
    bool operator()(const DBTypeDescription& lhs,
                    const DBTypeDescription& rhs) const {
      return lhs.name == rhs.name && lhs.schema == rhs.schema;
    }
  };
};

namespace io {

/// Oids are predefined for postgres fundamental types
/// Constants can be found here
/// https://github.com/postgres/postgres/blob/master/src/include/catalog/pg_type.dat
/// Oid values can also be retrieved from PostgreSQL instance by running query
/// @code
/// select t.oid as "Oid", pg_catalog.format_type(t.oid, null) as "Name"
/// from pg_catalog.pg_type t
///     left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
/// where (t.typrelid = 0 or (select c.relkind = 'c' from pg_catalog.pg_class c
/// where c.oid = t.typrelid))
///   and not exists(select 1 from pg_catalog.pg_type el where el.oid =
///   t.typelem and el.typarray = t.oid) and n.nspname = 'pg_catalog'
/// order by 1, 2
/// @endcode
enum class PredefinedOids {
  kInvalid = 0,
  kBoolean = 16,
  kBytea = 17,
  kChar = 18,
  kName = 19,
  kInt8 = 20,
  kInt2 = 21,
  kInt2Vector = 22,
  kInt4 = 23,
  kRegproc = 24,
  kText = 25,
  kOid = 26,
  kTid = 27,
  kXid = 28,
  kCid = 29,
  kOidVector = 30,
  kJson = 114,
  kXml = 142,
  kPgNodeTree = 194,
  kPgDdlCommand = 32,
  kPoint = 600,
  kLseg = 601,
  kPath = 602,
  kBox = 603,
  kPolygon = 604,
  kLine = 628,
  kFloat4 = 700,
  kFloat8 = 701,
  kAbstime = 702,
  kReltime = 703,
  kTinterval = 704,
  kUnknown = 705,
  kCircle = 718,
  kCash = 790,
  kMacaddr = 829,
  kInet = 869,
  kCidr = 650,
  kBooleanArray = 1000,  // Not in documentation
  kCharArray = 1002,     // Not in documentation
  kNameArray = 1003,     // Not in documentation
  kInt2Array = 1005,
  kInt4Array = 1007,
  kTextArray = 1009,
  kTidArray = 1010,      // Not in documentation
  kXidArray = 1011,      // Not in documentation
  kCidArray = 1012,      // Not in documentation
  kBphcarArray = 1014,   // Not in documentation
  kVarcharArray = 1015,  // Not in documentation
  kInt8Array = 1016,     // Not in documentation
  kFloat4Array = 1021,
  kFloat8Array = 1022,  // Not in documentation
  kOidArray = 1028,
  kAclItem = 1033,
  kCstringArray = 1263,
  kBpchar = 1042,
  kVarchar = 1043,
  kDate = 1082,
  kTime = 1083,
  kTimestamp = 1114,
  kTimestampArray = 1115,  // Not in documentation
  kTimestamptz = 1184,
  kTimestamptzArray = 1185,  // Not in documentation
  kInterval = 1186,
  kNumericArray = 1231,  // Not in documentation
  kTimetz = 1266,
  kBit = 1560,
  kVarbit = 1562,
  kNumeric = 1700,
  kRefcursor = 1790,
  kRegprocedure = 2202,
  kRegoper = 2203,
  kRegoperator = 2204,
  kRegclass = 2205,
  kRegtype = 2206,
  kRegrole = 4096,
  kRegtypearray = 2211,
  kUuid = 2950,
  kLsn = 3220,
  kTsvector = 3614,
  kGtsvector = 3642,
  kTsquery = 3615,
  kRegconfig = 3734,
  kRegdictionary = 3769,
  kJsonb = 3802,
  kInt4Range = 3904,
  kRecord = 2249,
  kRecordArray = 2287,
  kCstring = 2275,
  kAny = 2276,
  kAnyArray = 2277,
  kVoid = 2278,
  kTrigger = 2279,
  kEvttrigger = 3838,
  kLanguageHandler = 2280,
  kInternal = 2281,
  kOpaque = 2282,
  kAnyElement = 2283,
  kAnyNonArray = 2776,
  kAnyEnum = 3500,
  kFdwHandler = 3115,
  kAnyRange = 3831
};

template <PredefinedOids TypeOid>
using PredefinedOid = std::integral_constant<PredefinedOids, TypeOid>;

//@{
/** @name Array types for predefined oids */
template <PredefinedOids TypeOid>
struct ArrayType : PredefinedOid<PredefinedOids::kInvalid> {};

template <>
struct ArrayType<PredefinedOids::kBoolean>
    : PredefinedOid<PredefinedOids::kBoolean> {};
template <>
struct ArrayType<PredefinedOids::kChar>
    : PredefinedOid<PredefinedOids::kCharArray> {};
template <>
struct ArrayType<PredefinedOids::kName>
    : PredefinedOid<PredefinedOids::kNameArray> {};
template <>
struct ArrayType<PredefinedOids::kInt2>
    : PredefinedOid<PredefinedOids::kInt2Array> {};
template <>
struct ArrayType<PredefinedOids::kInt4>
    : PredefinedOid<PredefinedOids::kInt4Array> {};
template <>
struct ArrayType<PredefinedOids::kInt8>
    : PredefinedOid<PredefinedOids::kInt8Array> {};
template <>
struct ArrayType<PredefinedOids::kText>
    : PredefinedOid<PredefinedOids::kTextArray> {};
template <>
struct ArrayType<PredefinedOids::kOid>
    : PredefinedOid<PredefinedOids::kOidArray> {};
template <>
struct ArrayType<PredefinedOids::kTid>
    : PredefinedOid<PredefinedOids::kTidArray> {};
template <>
struct ArrayType<PredefinedOids::kXid>
    : PredefinedOid<PredefinedOids::kXidArray> {};
template <>
struct ArrayType<PredefinedOids::kCid>
    : PredefinedOid<PredefinedOids::kCidArray> {};

template <>
struct ArrayType<PredefinedOids::kFloat4>
    : PredefinedOid<PredefinedOids::kFloat4Array> {};
template <>
struct ArrayType<PredefinedOids::kFloat8>
    : PredefinedOid<PredefinedOids::kFloat8Array> {};

template <>
struct ArrayType<PredefinedOids::kBpchar>
    : PredefinedOid<PredefinedOids::kBphcarArray> {};
template <>
struct ArrayType<PredefinedOids::kVarchar>
    : PredefinedOid<PredefinedOids::kVarcharArray> {};

template <>
struct ArrayType<PredefinedOids::kTimestamp>
    : PredefinedOid<PredefinedOids::kTimestampArray> {};
template <>
struct ArrayType<PredefinedOids::kTimestamptz>
    : PredefinedOid<PredefinedOids::kTimestamptzArray> {};

template <>
struct ArrayType<PredefinedOids::kNumeric>
    : PredefinedOid<PredefinedOids::kNumericArray> {};
//@}

}  // namespace io
}  // namespace storages::postgres

namespace std {

template <>
struct hash<storages::postgres::io::PredefinedOids> {
  std::size_t operator()(storages::postgres::io::PredefinedOids value) const {
    return static_cast<std::size_t>(value);
  }
};

template <>
struct hash<storages::postgres::DBTypeName> {
  std::size_t operator()(const storages::postgres::DBTypeName& value) const {
    return value.GetHash();
  }
};

}  // namespace std
