#include <storages/postgres/io/chrono.hpp>

#include <unicode/timezone.h>

#include <logging/log.hpp>

namespace storages {
namespace postgres {

namespace {

namespace icu_ns = U_ICU_NAMESPACE;

// Not marked const because mktime requires a non-const pointer
std::tm kPgEpochTm{/* tm_sec */ 0,
                   /* tm_min */ 0,
                   /* tm_hour */ 0,
                   /* tm_mday */ 1,
                   /* tm_mon */ 0,
                   /* tm_year */ 100,
                   /* tm_wday */ 6 /* Saturday */,
                   /* tm_yday */ 0,
                   /* tm_isdst */ 0,
                   /* tm_gmtoff */ 0,
                   "UTC"};

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
  tz.getCanonicalID(id, canonical, err_code);
  if (U_FAILURE(err_code)) {
    LOG_TRACE() << "Error when getting canonical id for time zone " << result.id
                << ": " << u_errorName(err_code);
  }
  canonical.toUTF8String(result.canonical_id);
  return result;
}

}  // namespace

TimePoint PostgresEpoch() {
  return std::chrono::system_clock::from_time_t(std::mktime(&kPgEpochTm));
}

const TimeZoneID& LocalTimezoneID() {
  static TimeZoneID tz = GetLocalTimezoneID();
  return tz;
}

}  // namespace postgres
}  // namespace storages
