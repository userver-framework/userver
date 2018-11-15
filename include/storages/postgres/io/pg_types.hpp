#pragma once

#include <cstdint>

namespace storages {
namespace postgres {

/// PostgreSQL Oid type
/// unsigned int is used to match the type used in libpq
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
  kInt2Array = 1005,
  kInt4Array = 1007,
  kTextArray = 1009,
  kOidArray = 1028,
  kFloat4Array = 1021,
  kAclItem = 1033,
  kCstringArray = 1263,
  kBpchar = 1042,
  kVarchar = 1043,
  kDate = 1082,
  kTime = 1083,
  kTimestamp = 1114,
  kTimestamptz = 1184,
  kInterval = 1186,
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

}  // namespace io
}  // namespace postgres
}  // namespace storages

namespace std {

template <>
struct hash<storages::postgres::io::PredefinedOids> {
  std::size_t operator()(storages::postgres::io::PredefinedOids value) const {
    return static_cast<std::size_t>(value);
  }
};
}  // namespace std
