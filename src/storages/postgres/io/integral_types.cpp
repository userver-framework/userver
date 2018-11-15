#include <storages/postgres/io/integral_types.hpp>

#include <unordered_set>

namespace storages::postgres::io {

//@{
/** @name Register parsers for additional PG types mapped to integral types */
template <>
struct PgToCpp<PredefinedOids::kOid, Integer>
    : detail::PgToCppPredefined<PredefinedOids::kOid, Integer> {};
template <>
struct PgToCpp<PredefinedOids::kTid, Integer>
    : detail::PgToCppPredefined<PredefinedOids::kTid, Integer> {};
template <>
struct PgToCpp<PredefinedOids::kXid, Integer>
    : detail::PgToCppPredefined<PredefinedOids::kXid, Integer> {};
template <>
struct PgToCpp<PredefinedOids::kCid, Integer>
    : detail::PgToCppPredefined<PredefinedOids::kCid, Integer> {};
//@}

namespace {

const bool kReference =
    detail::ForceReference(PgToCpp<PredefinedOids::kOid, Integer>::init_,
                           PgToCpp<PredefinedOids::kTid, Integer>::init_,
                           PgToCpp<PredefinedOids::kXid, Integer>::init_,
                           PgToCpp<PredefinedOids::kCid, Integer>::init_);

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
