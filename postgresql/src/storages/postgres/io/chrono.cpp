#include <userver/storages/postgres/io/chrono.hpp>

#include <userver/logging/log_helper.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

logging::LogHelper& operator<<(logging::LogHelper& lh, TimePointTz tp) {
  lh << tp.GetUnderlying();
  return lh;
}

logging::LogHelper& operator<<(logging::LogHelper& lh, TimePointWithoutTz tp) {
  lh << tp.GetUnderlying();
  return lh;
}

std::ostream& operator<<(std::ostream& os, TimePointTz tp) {
  os << USERVER_NAMESPACE::utils::datetime::Timestring(tp.GetUnderlying());
  return os;
}

std::ostream& operator<<(std::ostream& os, TimePointWithoutTz tp) {
  os << USERVER_NAMESPACE::utils::datetime::Timestring(tp.GetUnderlying());
  return os;
}

TimePointTz Now() {
  return TimePointTz{USERVER_NAMESPACE::utils::datetime::Now()};
}

TimePointWithoutTz NowWithoutTz() {
  return TimePointWithoutTz{USERVER_NAMESPACE::utils::datetime::Now()};
}

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
