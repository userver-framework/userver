#include <userver/storages/postgres/io/integral_types.hpp>

#include <unordered_set>

#include <storages/postgres/io/pg_type_parsers.hpp>

USERVER_NAMESPACE_BEGIN

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

}  // namespace

void BufferParser<bool>::operator()(const FieldBuffer& buf) {
  if (buf.length != 1) {
    throw InvalidInputBufferSize{buf.length, "for boolean type"};
  }
  value = *buf.buffer != 0;
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
