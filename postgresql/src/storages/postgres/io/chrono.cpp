#include <userver/storages/postgres/io/chrono.hpp>

#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace io {

template <>
struct PgToCpp<PredefinedOids::kTimestamptz, TimePoint>
    : detail::PgToCppPredefined<PredefinedOids::kTimestamptz, TimePoint> {};

namespace {

const bool kReference = detail::ForceReference(
    PgToCpp<PredefinedOids::kTimestamptz, TimePoint>::init_);

}  // namespace
}  // namespace io

namespace {

// 01.01.2000 00:00:00 @ UTC, PostgreSQL epoch
const std::time_t kPgEpochTime = 946684800;
const auto kPgEpoch = std::chrono::system_clock::from_time_t(kPgEpochTime);

}  // namespace

TimePoint PostgresEpochTimePoint() { return kPgEpoch; }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
