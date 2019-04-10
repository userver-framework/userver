#include <storages/postgres/io/chrono.hpp>

#include <storages/postgres/io/type_mapping.hpp>

#include <unicode/timezone.h>

#include <logging/log.hpp>

namespace storages {
namespace postgres {

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

namespace icu_ns = U_ICU_NAMESPACE;

// 01.01.2000 00:00:00 @ UTC, PostgreSQL epoch
const std::time_t kPgEpochT = 946684800;
const auto kPgEpoch = std::chrono::system_clock::from_time_t(kPgEpochT);

/// Local ICU time zone. Might be useful to calculate current UTC offset.
icu_ns::TimeZone& GetICUTimezone() {
  static std::unique_ptr<icu_ns::TimeZone> icu_tz{
      icu_ns::TimeZone::createDefault()};
  return *icu_tz;
}

TimeZoneID GetLocalTimezoneID() {
  auto& tz = GetICUTimezone();
  TimeZoneID result;
  icu_ns::UnicodeString id;
  icu_ns::UnicodeString canonical;
  UErrorCode err_code{U_ZERO_ERROR};
  tz.getID(id);
  id.toUTF8String(result.id);
  icu_ns::TimeZone::getCanonicalID(id, canonical, err_code);
  if (U_FAILURE(err_code)) {
    LOG_TRACE() << "Error when getting canonical id for time zone " << result.id
                << ": " << u_errorName(err_code);
  }
  canonical.toUTF8String(result.canonical_id);
  return result;
}

}  // namespace

TimePoint PostgresEpoch() { return kPgEpoch; }

const TimeZoneID& LocalTimezoneID() {
  static TimeZoneID tz = GetLocalTimezoneID();
  return tz;
}

}  // namespace postgres
}  // namespace storages
