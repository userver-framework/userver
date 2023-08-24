#include <userver/storages/postgres/io/date.hpp>

#include <chrono>

#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace io {

template <>
struct PgToCpp<PredefinedOids::kDate, Date>
    : detail::PgToCppPredefined<PredefinedOids::kDate, Date> {};

namespace {

const auto kReference =
    detail::ForceReference(PgToCpp<PredefinedOids::kDate, Date>::init_);

}  // namespace
}  // namespace io

Date PostgresEpochDate() {
  static const Date kPgEpoch{2000, 1, 1};
  return kPgEpoch;
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
