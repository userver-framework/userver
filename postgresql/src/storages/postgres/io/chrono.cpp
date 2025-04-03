#include <userver/storages/postgres/io/chrono.hpp>

#include <userver/dump/common.hpp>
#include <userver/dump/operations.hpp>
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

TimePointTz Now() { return TimePointTz{USERVER_NAMESPACE::utils::datetime::Now()}; }

TimePointWithoutTz NowWithoutTz() { return TimePointWithoutTz{USERVER_NAMESPACE::utils::datetime::Now()}; }

namespace io {

template <>
struct PgToCpp<PredefinedOids::kTimestamptz, TimePoint>
    : detail::PgToCppPredefined<PredefinedOids::kTimestamptz, TimePoint> {};

namespace {

const bool kReference = detail::ForceReference(PgToCpp<PredefinedOids::kTimestamptz, TimePoint>::init_);

}  // namespace
}  // namespace io

void Write(dump::Writer& writer, const TimePointTz& value) { writer.Write(value.GetUnderlying()); }

TimePointTz Read(dump::Reader& reader, dump::To<TimePointTz>) {
    return TimePointTz{reader.Read<TimePointTz::UnderlyingType>()};
}

void Write(dump::Writer& writer, const TimePointWithoutTz& value) { writer.Write(value.GetUnderlying()); }

TimePointWithoutTz Read(dump::Reader& reader, dump::To<TimePointWithoutTz>) {
    return TimePointWithoutTz{reader.Read<TimePointWithoutTz::UnderlyingType>()};
}

namespace {

// 01.01.2000 00:00:00 @ UTC, PostgreSQL epoch
constexpr std::time_t kPgEpochTime = 946684800;
const auto kPgEpoch = std::chrono::system_clock::from_time_t(kPgEpochTime);

}  // namespace

TimePoint PostgresEpochTimePoint() { return kPgEpoch; }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
