#pragma once

/// @file userver/storages/postgres/io/chrono.hpp
/// @brief Timestamp (std::chrono::*) I/O support
/// @ingroup userver_postgres_parse_and_format

#include <chrono>
#include <limits>

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/interval.hpp>
#include <userver/storages/postgres/io/transform_io.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

using ClockType = std::chrono::system_clock;

/// @brief Corresponds to TIMESTAMP WITHOUT TIME ZONE (value in absolute time
/// (no TZ offset)).
using TimePoint = ClockType::time_point;

/// @brief Corresponds to TIMESTAMP WITH TIME ZONE (value in absolute time (no
/// TZ offset)). Time zones are applied automatically.
using TimePointTz = USERVER_NAMESPACE::utils::StrongTypedef<
    struct TimestampWithTz, TimePoint,
    USERVER_NAMESPACE::utils::StrongTypedefOps::kCompareTransparent>;
using IntervalType = std::chrono::microseconds;

/// Postgres epoch timestamp (2000-01-01 00:00 UTC)
TimePoint PostgresEpochTimePoint();

/// Constant equivalent to PostgreSQL 'infinity'::timestamp, a time point that
/// is later than all other time points
/// https://www.postgresql.org/docs/current/datatype-datetime.html#DATATYPE-DATETIME-SPECIAL-TABLE
inline constexpr TimePoint kTimestampPositiveInfinity = TimePoint::max();
/// Constant equivalent to PostgreSQL '-infinity'::timestamp, a time point that
/// is earlier than all other time points
/// https://www.postgresql.org/docs/current/datatype-datetime.html#DATATYPE-DATETIME-SPECIAL-TABLE
inline constexpr TimePoint kTimestampNegativeInfinity = TimePoint::min();

/**
 * @page pg_timestamp uPg timestamp support
 *
 * The driver provides mapping from C++ std::chrono::time_point template type to
 * Postgres timestamp (without time zone) data type.
 *
 * To read/write timestamp with time zone Postgres data type a TimePointTz
 * helper type is provided.
 *
 * Postgres internal timestamp resolution is microseconds.
 *
 * Note on time zones:
 * std::chrono::time_point is an absolute time and does not correspond to any
 * specific time zone. std::chrono::system_clock::now() and utils::Stringtime()
 * return this kind of time point. It is always sent to PG as such, and you can
 * think of it as an UTC time.
 *
 * Postgres allows implicit conversion between TIMESTAMP and TIMESTAMP WITH TIME
 * ZONE and it *does not apply any offsets* when doing so.
 *
 * Because of this you MUST ensure that you always use the correct type:
 *   * std::chrono::time_point for TIMESTAMP;
 *   * storages::postgres::TimePointTz for TIMESTAMP FOR TIME ZONE.
 * Otherwise, you'll get skewed times in database!
 *
 * @code
 * namespace pg = storages::postgres;
 *
 * pg::Transaction trx = ...;
 *
 * auto now = std::chrono::system_clock::now();
 * // Send as timestamp without time zone
 * auto res = trx.Execute("select $1", now);
 * // Send as timestamp with time zone
 * res = trx.Execute("select $1", pg::TimePointTz{now});
 * // Read as timestamp
 * res[0].To(now);
 * @endcode
 */

namespace io {

/// @brief Binary formatter for std::chrono::time_point.
template <typename Duration>
struct BufferFormatter<std::chrono::time_point<ClockType, Duration>> {
  using ValueType = std::chrono::time_point<ClockType, Duration>;

  const ValueType value;

  explicit BufferFormatter(ValueType val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    static const ValueType pg_epoch =
        std::chrono::time_point_cast<Duration>(PostgresEpochTimePoint());
    if (value == kTimestampPositiveInfinity) {
      WriteBuffer(types, buf, std::numeric_limits<Bigint>::max());
    } else if (value == kTimestampNegativeInfinity) {
      WriteBuffer(types, buf, std::numeric_limits<Bigint>::min());
    } else {
      auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(value -
                                                                       pg_epoch)
                     .count();
      WriteBuffer(types, buf, tmp);
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
        std::chrono::time_point_cast<Duration>(PostgresEpochTimePoint());
    Bigint usec{0};
    ReadBuffer(buffer, usec);
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
}  // namespace storages::postgres

USERVER_NAMESPACE_END
