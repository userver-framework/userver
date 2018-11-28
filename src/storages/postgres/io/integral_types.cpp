#include <storages/postgres/io/integral_types.hpp>

#include <unordered_set>

#include <storages/postgres/io/pg_type_parsers.hpp>

namespace storages::postgres::io {

//@{
/** @name Register parsers for additional PG types mapped to integral types */
template <>
struct PgToCpp<PredefinedOids::kOid, Oid>
    : detail::PgToCppPredefined<PredefinedOids::kOid, Oid> {};
template <>
struct PgToCpp<PredefinedOids::kTid, Oid>
    : detail::PgToCppPredefined<PredefinedOids::kTid, Oid> {};
template <>
struct PgToCpp<PredefinedOids::kXid, Oid>
    : detail::PgToCppPredefined<PredefinedOids::kXid, Oid> {};
template <>
struct PgToCpp<PredefinedOids::kCid, Oid>
    : detail::PgToCppPredefined<PredefinedOids::kCid, Oid> {};
//@}

namespace {

const bool kReference = detail::ForceReference(
    CppToPg<Oid>::init_, CppToPg<Smallint>::init_, CppToPg<Integer>::init_,
    CppToPg<Bigint>::init_, CppToPg<bool>::init_,
    PgToCpp<PredefinedOids::kOid, Oid>::init_,
    PgToCpp<PredefinedOids::kTid, Oid>::init_,
    PgToCpp<PredefinedOids::kXid, Oid>::init_,
    PgToCpp<PredefinedOids::kCid, Oid>::init_);

bool IsTrueLiteral(const std::string& lit) {
  static const std::unordered_set<std::string> kTrueLiterals{
      "TRUE", "t", "true", "y", "yes", "on", "1"};
  return kTrueLiterals.count(lit);
}

const std::unordered_set<std::string> kFalseLiterals{
    "FALSE", "f", "false", "n", "no", "off", "0"};

}  // namespace

void BufferParser<bool, DataFormat::kTextDataFormat>::operator()(
    const FieldBuffer& buf) {
  std::string bool_literal(buf.buffer, buf.length);
  value = IsTrueLiteral(bool_literal);
}

}  // namespace storages::postgres::io
