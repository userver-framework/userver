#pragma once

/// @file storages/postgres/io/chrono.hpp
/// @brief Timestamp I/O support

#include <chrono>
#include <string>

#include <cctz/time_zone.h>

#include <compiler/demangle.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/interval.hpp>
#include <storages/postgres/io/transform_io.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <utils/datetime.hpp>
#include <utils/strong_typedef.hpp>

namespace storages {
namespace postgres {

using ClockType = std::chrono::system_clock;
using TimePoint = ClockType::time_point;
/// Strong typedef for chrono time_point to be used in conjunction with
/// timestamptz
using TimePointTz =
    ::utils::StrongTypedef<struct TimestampWithTz, TimePoint,
                           ::utils::StrongTypedefOps::kCompareTransparent>;
using IntervalType = std::chrono::microseconds;

TimePoint PostgresEpoch();

/// Constant equivalent to PostgreSQL 'infinity'::timestamp, a time point that
/// is later than all other time points
/// https://www.postgresql.org/docs/10/datatype-datetime.html#DATATYPE-DATETIME-SPECIAL-TABLE
const TimePoint kTimestampPositiveInfinity = TimePoint::max();
/// Constant equivalent to PostgreSQL '-infinity'::timestamp, a time point that
/// is earlier than all other time points
/// https://www.postgresql.org/docs/10/datatype-datetime.html#DATATYPE-DATETIME-SPECIAL-TABLE
const TimePoint kTimestampNegativeInfinity = TimePoint::min();

struct TimeZoneID {
  std::string id;
  std::string canonical_id;
};

/// @brief Get local time zone ID and it's canonical counterpart.
/// Apparently, the time zone won't change without a restart, so it's computed
/// only once.
const TimeZoneID& LocalTimezoneID();

/**
 * @page pg_timestamp µPg timestamp support
 *
 * The driver provides mapping from C++ std::chrono::time_point template type to
 * Postgres timestamp (without time zone) data type.
 *
 * The value is formatted/parsed using UTC time zone. After that the local TZ
 * offset is applied.
 *
 * To read/write timestamp with time zone Postgres data type a helper type is
 * provided.
 *
 * Please note that Postgres timestamp resolution is microseconds.
 *
 * @code
 * namespace pg = storages::postgres;
 *
 * pg::Transaction trx = ...;
 *
 * auto now = std::chrono::system_clock::now();
 * // Send as timestamp
 * auto res = trx.Execute("select $1", now);
 * // Read as timestamp
 * res[0].To(now);
 * // Send as timestamp with time zone, in time zone passed as the second
 * // argument, defaults to local TZ
 * res = trx.Execute("select $1", pg::TimestampTz(now, cctz::utc_time_zone()));
 * // Read as timestamp with time zone
 * res[0].To(pg::TimestampTz(now, cctz::utc_time_zone());
 * @endcode
 */

namespace detail {

template <typename TimePoint>
struct TimestampTz;

/// Helper structure for reading a time point with timezone
template <typename Duration>
struct TimestampTz<std::chrono::time_point<ClockType, Duration>> {
  using TimePointType = std::chrono::time_point<ClockType, Duration>;
  TimePointType& value;
  cctz::time_zone tz;
};

/// Helper structure for writing a time point with timezone
template <typename Duration>
struct TimestampTz<const std::chrono::time_point<ClockType, Duration>> {
  using TimePointType = std::chrono::time_point<ClockType, Duration>;
  const TimePointType& value;
  cctz::time_zone tz;
};

}  // namespace detail

/// Helper function for creating an input structure for reading timestamp
/// with timezone
/// @code
/// namespace pg = storages::postgres;
/// std::chrono::system_clock::time_point tp;
/// result.To(pg::TimespamptTz(tp));
/// @endcode
template <typename Duration>
detail::TimestampTz<std::chrono::time_point<ClockType, Duration>> TimestampTz(
    std::chrono::time_point<ClockType, Duration>& val,
    const cctz::time_zone& tz) {
  return {val, tz};
}

/// Helper function for creating an output structure for writing timestamp
/// with timezone
/// @code
/// namespace pg = storages::postgres;
/// std::chrono::system_clock::time_point tp;
/// trx.Execute("select $1", pg::TimestampTz(tp));
/// @endcode
template <typename Duration>
detail::TimestampTz<const std::chrono::time_point<ClockType, Duration>>
TimestampTz(const std::chrono::time_point<ClockType, Duration>& val,
            const cctz::time_zone& tz) {
  return {val, tz};
}

template <typename Duration>
detail::TimestampTz<std::chrono::time_point<ClockType, Duration>> TimestampTz(
    std::chrono::time_point<ClockType, Duration>& val) {
  return {val, cctz::local_time_zone()};
}

template <typename Duration>
detail::TimestampTz<const std::chrono::time_point<ClockType, Duration>>
TimestampTz(const std::chrono::time_point<ClockType, Duration>& val) {
  return {val, cctz::local_time_zone()};
}

std::string Timestring(
    TimePointTz tp,
    const std::string& timezone = ::utils::datetime::kDefaultTimezone,
    const std::string& format = ::utils::datetime::kDefaultFormat);

namespace io {

/// @brief Binary formatter for timestamptz.
/// PostgreSQL expects timestamptz in UTC when in binary format.
template <typename TimePointType>
struct BufferFormatter<postgres::detail::TimestampTz<TimePointType>> {
  using ValueType = postgres::detail::TimestampTz<TimePointType>;
  ValueType value;

  explicit BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    auto lookup = value.tz.lookup(value.value);
    auto val = value.value - std::chrono::seconds{lookup.offset};
    io::WriteBuffer(types, buf, val);
  }
};

/// @brief Binary parser for timestamptz.
///
template <typename Duration>
struct BufferParser<
    postgres::detail::TimestampTz<std::chrono::time_point<ClockType, Duration>>>
    : detail::BufferParserBase<postgres::detail::TimestampTz<
          std::chrono::time_point<ClockType, Duration>>&&> {
  using BaseType = detail::BufferParserBase<postgres::detail::TimestampTz<
      std::chrono::time_point<ClockType, Duration>>&&>;
  using ValueType = typename BaseType::ValueType;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    auto lookup = this->value.tz.lookup(this->value.value);
    typename ValueType::TimePointType tmp;
    io::ReadBuffer(buffer, tmp);
    this->value.value = tmp + std::chrono::seconds{lookup.offset};
  }
};

/// @brief Binary formatter for std::chrono::time_point.
template <typename Duration>
struct BufferFormatter<std::chrono::time_point<ClockType, Duration>> {
  using ValueType = std::chrono::time_point<ClockType, Duration>;

  const ValueType& value;

  explicit BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    static const ValueType pg_epoch =
        std::chrono::time_point_cast<Duration>(PostgresEpoch());
    if (value == kTimestampPositiveInfinity) {
      io::WriteBuffer(types, buf, std::numeric_limits<Bigint>::max());
    } else if (value == kTimestampNegativeInfinity) {
      io::WriteBuffer(types, buf, std::numeric_limits<Bigint>::min());
    } else {
      auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(value -
                                                                       pg_epoch)
                     .count();
      io::WriteBuffer(types, buf, tmp);
    }
  }
};

/// @brief Binary parser for std::chrono::time_point.
template <typename Duration>
struct BufferParser<std::chrono::time_point<ClockType, Duration>>
    : detail::BufferParserBase<std::chrono::time_point<ClockType, Duration>> {
  using BaseType =
      detail::BufferParserBase<std::chrono::time_point<ClockType, Duration>>;
  using ValueType = typename BaseType::ValueType;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    static const ValueType pg_epoch =
        std::chrono::time_point_cast<Duration>(PostgresEpoch());
    Bigint usec{0};
    io::ReadBuffer(buffer, usec);
    if (usec == std::numeric_limits<Bigint>::max()) {
      this->value = kTimestampPositiveInfinity;
    } else if (usec == std::numeric_limits<Bigint>::min()) {
      this->value = kTimestampNegativeInfinity;
    } else {
      ValueType tmp = pg_epoch + std::chrono::microseconds{usec};
      std::swap(tmp, this->value);
    }
  }
};

namespace detail {

template <typename Rep, typename Period>
struct DurationIntervalCvt {
  using UserType = std::chrono::duration<Rep, Period>;
  UserType operator()(const Interval& wire_val) const {
    return std::chrono::duration_cast<UserType>(wire_val.GetDuration());
  }
  Interval operator()(const UserType& user_val) const {
    return Interval{std::chrono::duration_cast<IntervalType>(user_val)};
  }
};

}  // namespace detail

namespace traits {

/// @brief Binary formatter for std::chrono::duration
template <typename Rep, typename Period>
struct Output<std::chrono::duration<Rep, Period>> {
  using type = TransformFormatter<std::chrono::duration<Rep, Period>,
                                  io::detail::Interval,
                                  io::detail::DurationIntervalCvt<Rep, Period>>;
};

/// @brief Binary parser for std::chrono::duration
template <typename Rep, typename Period>
struct Input<std::chrono::duration<Rep, Period>> {
  using type =
      TransformParser<std::chrono::duration<Rep, Period>, io::detail::Interval,
                      io::detail::DurationIntervalCvt<Rep, Period>>;
};

}  // namespace traits

template <typename TimePointType>
struct CppToSystemPg<postgres::detail::TimestampTz<TimePointType>>
    : PredefinedOid<PredefinedOids::kTimestamptz> {};

template <>
struct CppToSystemPg<TimePointTz>
    : PredefinedOid<PredefinedOids::kTimestamptz> {};

template <typename Duration>
struct CppToSystemPg<std::chrono::time_point<ClockType, Duration>>
    : PredefinedOid<PredefinedOids::kTimestamp> {};

template <typename Rep, typename Period>
struct CppToSystemPg<std::chrono::duration<Rep, Period>>
    : PredefinedOid<PredefinedOids::kInterval> {};

}  // namespace io
}  // namespace postgres
}  // namespace storages
