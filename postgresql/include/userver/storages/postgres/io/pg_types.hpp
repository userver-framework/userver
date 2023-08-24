#pragma once

/// @file userver/storages/postgres/io/pg_types.hpp
/// @brief Postgres-specific types I/O support

#include <cstdint>
#include <string>
#include <string_view>

#include <userver/storages/postgres/detail/db_data_type_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// PostgreSQL Oid type.
// Aliased to unsigned int to match the type used in libpq.
using Oid = unsigned int;
inline constexpr Oid kInvalidOid = 0;

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
  const std::string_view schema;
  const std::string_view name;

  constexpr DBTypeName() : schema{}, name{} {}
  explicit constexpr DBTypeName(std::pair<std::string_view, std::string_view> n)
      : schema(n.first), name(n.second) {}
  /// Implicit constructor from a string literal, to enable declarations like
  /// @code
  /// DBTypeName my_type = "my_schema.my_type";
  /// @endcode
  /* implicit */ constexpr DBTypeName(const char* name)
      : DBTypeName(utils::ParseDBName(name)) {}
  constexpr DBTypeName(std::string_view s, std::string_view n)
      : schema(s), name(n) {}

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
  std::string ToString() const;
};

/// @brief Description of a PostgreSQL type.
/// The structure is selected from the pg_catalog.pg_type table (not all, only
/// appropriate fields).
/// See https://www.postgresql.org/docs/12/catalog-pg-type.html
struct DBTypeDescription {
  enum class TypeClass : char {
    kUnknown = 'X',
    kBase = 'b',
    kComposite = 'c',
    kDomain = 'd',
    kEnum = 'e',
    kPseudo = 'p',
    kRange = 'r'
  };
  /// @brief PosgtreSQL type category.
  /// See
  /// https://www.postgresql.org/docs/12/catalog-pg-type.html#CATALOG-TYPCATEGORY-TABLE
  enum class TypeCategory : char {
    kInvalid = 0,  //!< Invalid type category
    kArray = 'A',  //!< https://www.postgresql.org/docs/12/arrays.html
    kBoolean =
        'B',  //!< https://www.postgresql.org/docs/12/datatype-boolean.html
    kComposite = 'C',  //!< https://www.postgresql.org/docs/12/rowtypes.html
    kDatetime =
        'D',  //!< https://www.postgresql.org/docs/12/datatype-datetime.html
    kEnumeration =
        'E',  //!< https://www.postgresql.org/docs/12/datatype-enum.html
    kGeometric =
        'G',  //!< https://www.postgresql.org/docs/12/datatype-geometric.html
    kNetwork =
        'I',  //!< https://www.postgresql.org/docs/12/datatype-net-types.html
    kNumeric =
        'N',  //!< https://www.postgresql.org/docs/12/datatype-numeric.html
    kPseudotype =
        'P',       //!< https://www.postgresql.org/docs/12/datatype-pseudo.html
    kRange = 'R',  //!< https://www.postgresql.org/docs/12/rangetypes.html
    kString =
        'S',  //!< https://www.postgresql.org/docs/12/datatype-character.html
    kTimespan =
        'T',      //!< https://www.postgresql.org/docs/12/datatype-datetime.html
    kUser = 'U',  //!< User types, created by fourth form of create type
                  /// statement
                  /// https://www.postgresql.org/docs/12/sql-createtype.html
    kBitstring = 'V',  //!< https://www.postgresql.org/docs/12/datatype-bit.html
    kUnknown = 'X'     //!< Unknown type category
  };

  /// pg_type.oid
  Oid oid{kInvalidOid};
  /// pg_namespace.nspname
  std::string schema;
  /// pg_type.typname
  std::string name;
  /// pg_type.typlen
  Integer length{0};
  /// pg_type.typtype
  TypeClass type_class{TypeClass::kUnknown};
  /// pg_type.typcategory
  TypeCategory category{TypeCategory::kInvalid};
  /// pg_type.typrelid
  Oid relation_id{kInvalidOid};
  /// pg_type.typelem
  /// If not zero, this type is an array
  Oid element_type{kInvalidOid};
  /// pg_type.typarray
  Oid array_type{kInvalidOid};
  /// pg_type.typbasetype
  /// Base type for domains
  Oid base_type{kInvalidOid};
  /// pg_type.typnotnull
  /// Used only with domains
  bool not_null{false};
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
  struct TypeCategoryHash {
    using IntegralType = std::underlying_type_t<TypeCategory>;
    auto operator()(TypeCategory val) const {
      return std::hash<IntegralType>{}(static_cast<IntegralType>(val));
    }
  };
};

/// Description of a field in a user-defined composite type, for type checking
struct CompositeFieldDef {
  Oid owner{kInvalidOid};
  std::string name;
  Oid type{kInvalidOid};

  static CompositeFieldDef EmptyDef() { return {kInvalidOid, {}, kInvalidOid}; }
};

std::string ToString(DBTypeDescription::TypeClass);

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
  kInvalid = kInvalidOid,
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
  kJsonArray = 199,  // Not in documentation
  kXml = 142,
  kPgNodeTree = 194,
  kPgDdlCommand = 32,
  kPoint = 600,
  kLseg = 601,
  kPath = 602,
  kBox = 603,
  kPolygon = 604,
  kLine = 628,
  kLineArray = 629,
  kFloat4 = 700,
  kFloat8 = 701,
  kAbstime = 702,
  kReltime = 703,
  kTinterval = 704,
  kUnknown = 705,
  kCircle = 718,
  kCircleArray = 719,  // Not in documentation
  kCash = 790,
  kMacaddr = 829,
  kInet = 869,
  kCidr = 650,
  kBooleanArray = 1000,  // Not in documentation
  kByteaArray = 1001,    // Not in documentation
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
  kPointArray = 1017,    // Not in documentation
  kLsegArray = 1018,     // Not in documentation
  kPathArray = 1019,     // Not in documentation
  kBoxArray = 1020,      // Not in documentation
  kFloat4Array = 1021,
  kFloat8Array = 1022,   // Not in documentation
  kPolygonArray = 1027,  // Not in documentation
  kOidArray = 1028,
  kAclItem = 1033,
  kCstringArray = 1263,
  kBpchar = 1042,
  kVarchar = 1043,
  kDate = 1082,
  kTime = 1083,
  kDateArray = 1182,  // Not in documentation
  kTimeArray = 1183,  // Not in documentation
  kTimestamp = 1114,
  kTimestampArray = 1115,  // Not in documentation
  kTimestamptz = 1184,
  kTimestamptzArray = 1185,  // Not in documentation
  kInterval = 1186,
  kIntervalArray = 1187,  // Not in documentation
  kNumericArray = 1231,   // Not in documentation
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
  kUuidArray = 2951,  // Not in documentation
  kLsn = 3220,
  kLsnArray = 3221,
  kTsvector = 3614,
  kGtsvector = 3642,
  kTsquery = 3615,
  kRegconfig = 3734,
  kRegdictionary = 3769,
  kJsonb = 3802,
  kJsonbArray = 3807,  // Not in documentation
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
  kAnyRange = 3831,
  kInt4Range = 3904,
  kInt4RangeArray = 3905,         // Not in documentation
  kNumRange = 3906,               // Not in documentation
  kNumRangeArray = 3907,          // Not in documentation
  kTimestampRange = 3908,         // Not in documentation
  kTimestampRangeArray = 3909,    // Not in documentation
  kTimestamptzRange = 3910,       // Not in documentation
  kTimestamptzRangeArray = 3911,  // Not in documentation
  kDateRange = 3912,              // Not in documentation
  kDateRangeArray = 3913,         // Not in documentation
  kInt8Range = 3926,              // Not in documentation
  kInt8RangeArray = 3927,         // Not in documentation
};

template <PredefinedOids TypeOid>
using PredefinedOid = std::integral_constant<PredefinedOids, TypeOid>;

//@{
/** @name Array types for predefined oids */
template <PredefinedOids TypeOid>
struct ArrayType : PredefinedOid<PredefinedOids::kInvalid> {};

template <>
struct ArrayType<PredefinedOids::kBoolean>
    : PredefinedOid<PredefinedOids::kBooleanArray> {};
template <>
struct ArrayType<PredefinedOids::kBytea>
    : PredefinedOid<PredefinedOids::kByteaArray> {};
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
struct ArrayType<PredefinedOids::kLsn>
    : PredefinedOid<PredefinedOids::kLsnArray> {};

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
struct ArrayType<PredefinedOids::kDate>
    : PredefinedOid<PredefinedOids::kDateArray> {};

template <>
struct ArrayType<PredefinedOids::kTime>
    : PredefinedOid<PredefinedOids::kTimeArray> {};

template <>
struct ArrayType<PredefinedOids::kTimestamp>
    : PredefinedOid<PredefinedOids::kTimestampArray> {};
template <>
struct ArrayType<PredefinedOids::kTimestamptz>
    : PredefinedOid<PredefinedOids::kTimestamptzArray> {};

template <>
struct ArrayType<PredefinedOids::kInterval>
    : PredefinedOid<PredefinedOids::kIntervalArray> {};

template <>
struct ArrayType<PredefinedOids::kNumeric>
    : PredefinedOid<PredefinedOids::kNumericArray> {};

template <>
struct ArrayType<PredefinedOids::kUuid>
    : PredefinedOid<PredefinedOids::kUuidArray> {};

template <>
struct ArrayType<PredefinedOids::kInt4Range>
    : PredefinedOid<PredefinedOids::kInt4RangeArray> {};

template <>
struct ArrayType<PredefinedOids::kNumRange>
    : PredefinedOid<PredefinedOids::kNumRangeArray> {};

template <>
struct ArrayType<PredefinedOids::kTimestampRange>
    : PredefinedOid<PredefinedOids::kTimestampRangeArray> {};

template <>
struct ArrayType<PredefinedOids::kTimestamptzRange>
    : PredefinedOid<PredefinedOids::kTimestamptzRangeArray> {};

template <>
struct ArrayType<PredefinedOids::kDateRange>
    : PredefinedOid<PredefinedOids::kDateRangeArray> {};

template <>
struct ArrayType<PredefinedOids::kInt8Range>
    : PredefinedOid<PredefinedOids::kInt8RangeArray> {};

template <>
struct ArrayType<PredefinedOids::kPoint>
    : PredefinedOid<PredefinedOids::kPointArray> {};
template <>
struct ArrayType<PredefinedOids::kLseg>
    : PredefinedOid<PredefinedOids::kLsegArray> {};
template <>
struct ArrayType<PredefinedOids::kLine>
    : PredefinedOid<PredefinedOids::kLineArray> {};
template <>
struct ArrayType<PredefinedOids::kBox>
    : PredefinedOid<PredefinedOids::kBoxArray> {};
template <>
struct ArrayType<PredefinedOids::kPath>
    : PredefinedOid<PredefinedOids::kPathArray> {};
template <>
struct ArrayType<PredefinedOids::kPolygon>
    : PredefinedOid<PredefinedOids::kPolygonArray> {};
template <>
struct ArrayType<PredefinedOids::kCircle>
    : PredefinedOid<PredefinedOids::kCircleArray> {};

template <>
struct ArrayType<PredefinedOids::kJson>
    : PredefinedOid<PredefinedOids::kJsonArray> {};
template <>
struct ArrayType<PredefinedOids::kJsonb>
    : PredefinedOid<PredefinedOids::kJsonbArray> {};

template <>
struct ArrayType<PredefinedOids::kRecord>
    : PredefinedOid<PredefinedOids::kRecordArray> {};
//@}

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END

namespace std {

template <>
struct hash<USERVER_NAMESPACE::storages::postgres::io::PredefinedOids> {
  std::size_t operator()(
      USERVER_NAMESPACE::storages::postgres::io::PredefinedOids value) const {
    return static_cast<std::size_t>(value);
  }
};

template <>
struct hash<USERVER_NAMESPACE::storages::postgres::DBTypeName> {
  std::size_t operator()(
      const USERVER_NAMESPACE::storages::postgres::DBTypeName& value) const {
    return value.GetHash();
  }
};

}  // namespace std
