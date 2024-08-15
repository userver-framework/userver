#pragma once

/// @file userver/storages/postgres/io/chrono.hpp
/// @brief Timestamp (std::chrono::*) I/O support
/// @ingroup userver_postgres_parse_and_format

#include <chrono>
#include <functional>
#include <iosfwd>
#include <limits>

#include <userver/logging/log_helper_fwd.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/interval.hpp>
#include <userver/storages/postgres/io/transform_io.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace detail {
struct TimePointTzTag {};
struct TimePointWithoutTzTag {};
}  // namespace detail

using ClockType = std::chrono::system_clock;
using TimePoint = ClockType::time_point;
using IntervalType = std::chrono::microseconds;

/// @brief Corresponds to TIMESTAMP WITH TIME ZONE (value in absolute time (no
/// TZ offset)). Time zones are applied automatically.
///
/// @warning It's important that no conversion to TIMESTAMP WITHOUT TIME ZONE is
/// performed on the DB side, see @ref pg_timestamp
///
/// @see @ref Now
struct TimePointTz final
    : public USERVER_NAMESPACE::utils::StrongTypedef<
          detail::TimePointTzTag, TimePoint,
          USERVER_NAMESPACE::utils::StrongTypedefOps::kCompareTransparent> {
  using StrongTypedef::StrongTypedef;

  /*implicit*/ operator TimePoint() const { return GetUnderlying(); }
};

/// @brief Logging support for TimePointTz.
logging::LogHelper& operator<<(logging::LogHelper&, TimePointTz);

/// @brief gtest logging support for TimePointTz.
std::ostream& operator<<(std::ostream&, TimePointTz);

/// @brief Corresponds to TIMESTAMP WITHOUT TIME ZONE (value in absolute time
/// (no TZ offset)).
///
/// @warning It's important that no conversion to TIMESTAMP WITH TIME ZONE is
/// performed on the DB side, see @ref pg_timestamp
///
/// @see @ref NowWithoutTz
struct TimePointWithoutTz final
    : public USERVER_NAMESPACE::utils::StrongTypedef<
          detail::TimePointWithoutTzTag, TimePoint,
          USERVER_NAMESPACE::utils::StrongTypedefOps::kCompareTransparent> {
  using StrongTypedef::StrongTypedef;

  /*implicit*/ operator TimePoint() const { return GetUnderlying(); }
};

/// @brief Logging support for TimePointWithoutTz.
logging::LogHelper& operator<<(logging::LogHelper&, TimePointWithoutTz);

/// @brief gtest logging support for TimePointWithoutTz.
std::ostream& operator<<(std::ostream&, TimePointWithoutTz);

/// @brief @ref utils::datetime::Now "Mockable" now that is written as
/// `TIMESTAMP WITH TIME ZONE`.
///
/// @see pg_timestamp
TimePointTz Now();

/// @brief @ref utils::datetime::Now "Mockable" now that is written as
/// `TIMESTAMP WITHOUT TIME ZONE`.
///
/// @see pg_timestamp
TimePointWithoutTz NowWithoutTz();

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
 * **Example:**
 *
 * @code
 * namespace pg = storages::postgres;
 *
 * pg::Transaction trx = ...;
 *
 * auto now = std::chrono::system_clock::now();
 * // Send as TIMESTAMP WITH TIME ZONE
 * res = trx.Execute("select $1", pg::TimePointTz{now});
 * // Send as TIMESTAMP WITHOUT TIME ZONE
 * auto res = trx.Execute("select $1", pg::TimePointWithoutTz{now});
 * // For reading, there is no difference between these time types
 * res[0].To(now);
 * @endcode
 *
 * ## How not to get skewed times in the database
 *
 * Postgres has two types corresponding to absolute time
 * (a.k.a. global time; Unix time; NOT local time):
 *   * `TIMESTAMP WITH TIME ZONE`
 *   * `TIMESTAMP`
 *
 * An unfortunate design decision on the Postgres side is that it allows
 * implicit conversion between them, and it applies an offset to the time point
 * when doing so, depending on the timezone of the Postgres database.
 *
 * Because of this you MUST ensure that you always use the correct type:
 *   * storages::postgres::TimePointTz for `TIMESTAMP WITH TIME ZONE`;
 *   * storages::postgres::TimePointWithoutTz for `TIMESTAMP`.
 *
 * Otherwise, you'll get skewed times in database!
 */

namespace io {

namespace detail {

template <typename Duration, typename Buffer>
void DoFormatTimePoint(std::chrono::time_point<ClockType, Duration> value,
                       const UserTypes& types, Buffer& buf) {
  static const auto pg_epoch =
      std::chrono::time_point_cast<Duration>(PostgresEpochTimePoint());
  if (value == kTimestampPositiveInfinity) {
    WriteBuffer(types, buf, std::numeric_limits<Bigint>::max());
  } else if (value == kTimestampNegativeInfinity) {
    WriteBuffer(types, buf, std::numeric_limits<Bigint>::min());
  } else {
    const auto tmp =
        std::chrono::duration_cast<std::chrono::microseconds>(value - pg_epoch)
            .count();
    WriteBuffer(types, buf, tmp);
  }
}

template <typename Duration>
void DoParseTimePoint(std::chrono::time_point<ClockType, Duration>& value,
                      const FieldBuffer& buffer) {
  static const auto pg_epoch =
      std::chrono::time_point_cast<Duration>(PostgresEpochTimePoint());
  Bigint usec{0};
  ReadBuffer(buffer, usec);
  if (usec == std::numeric_limits<Bigint>::max()) {
    value = kTimestampPositiveInfinity;
  } else if (usec == std::numeric_limits<Bigint>::min()) {
    value = kTimestampNegativeInfinity;
  } else {
    value = pg_epoch + std::chrono::microseconds{usec};
  }
}

template <typename T>
struct TimePointStrongTypedefFormatter {
  const T value;

  explicit TimePointStrongTypedefFormatter(T val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    detail::DoFormatTimePoint(value.GetUnderlying(), types, buf);
  }
};

template <typename T>
struct TimePointStrongTypedefParser : BufferParserBase<T> {
  using BufferParserBase<T>::BufferParserBase;

  void operator()(const FieldBuffer& buffer) {
    detail::DoParseTimePoint(this->value.GetUnderlying(), buffer);
  }
};

}  // namespace detail

/// @brief Binary formatter for TimePointTz.
template <>
struct BufferFormatter<TimePointTz>
    : detail::TimePointStrongTypedefFormatter<TimePointTz> {
  using TimePointStrongTypedefFormatter::TimePointStrongTypedefFormatter;
};

/// @brief Binary formatter for TimePointWithoutTz.
template <>
struct BufferFormatter<TimePointWithoutTz>
    : detail::TimePointStrongTypedefFormatter<TimePointWithoutTz> {
  using TimePointStrongTypedefFormatter::TimePointStrongTypedefFormatter;
};

/// @brief Binary parser for TimePointTz.
template <>
struct BufferParser<TimePointTz>
    : detail::TimePointStrongTypedefParser<TimePointTz> {
  using TimePointStrongTypedefParser::TimePointStrongTypedefParser;
};

/// @brief Binary parser for TimePointWithoutTz.
template <>
struct BufferParser<TimePointWithoutTz>
    : detail::TimePointStrongTypedefParser<TimePointWithoutTz> {
  using TimePointStrongTypedefParser::TimePointStrongTypedefParser;
};

/// @cond

// @brief Binary formatter for TimePoint. Implicitly converts
// TimePointWithoutTz to TimePoint.
template <>
struct BufferFormatter<TimePoint> {
  using ValueType = TimePoint;

  const ValueType value;

  explicit BufferFormatter(ValueType val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
#if !USERVER_POSTGRES_ENABLE_LEGACY_TIMESTAMP
    static_assert(sizeof(Buffer) == 0,
                  "====================> userver: Writing "
                  "std::chrono::system_clock::time_point is not supported. "
                  "Rewrite using the TimePointWithoutTz or TimePointTz types, "
                  "or define USERVER_POSTGRES_ENABLE_LEGACY_TIMESTAMP to 1.");
#endif
    detail::DoFormatTimePoint(value, types, buf);
  }
};

/// @endcond

/// @brief Binary parser for TimePoint. Implicitly converts TimePoint
/// to TimePointWithoutTz.
template <>
struct BufferParser<TimePoint> : detail::BufferParserBase<TimePoint> {
  using BufferParserBase::BufferParserBase;

  void operator()(const FieldBuffer& buffer) {
    detail::DoParseTimePoint(this->value, buffer);
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

template <>
struct CppToSystemPg<TimePointWithoutTz>
    : PredefinedOid<PredefinedOids::kTimestamp> {};

// For raw time_point, only parsing is normally allowed, not serialization.
template <typename Duration>
struct CppToSystemPg<std::chrono::time_point<ClockType, Duration>>
    : PredefinedOid<PredefinedOids::kTimestamp> {};

template <typename Rep, typename Period>
struct CppToSystemPg<std::chrono::duration<Rep, Period>>
    : PredefinedOid<PredefinedOids::kInterval> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END

/// @brief `std::hash` support for storages::postgres::TimePointTz.
template <>
struct std::hash<USERVER_NAMESPACE::storages::postgres::TimePointTz> {
  std::size_t operator()(
      const USERVER_NAMESPACE::storages::postgres::TimePointTz& v)  //
      const noexcept {
    return std::hash<
        USERVER_NAMESPACE::storages::postgres::TimePoint::duration::rep>{}(
        v.GetUnderlying().time_since_epoch().count());
  }
};

/// @brief `std::hash` support for storages::postgres::TimePointWithoutTz.
template <>
struct std::hash<USERVER_NAMESPACE::storages::postgres::TimePointWithoutTz> {
  std::size_t operator()(
      const USERVER_NAMESPACE::storages::postgres::TimePointWithoutTz& v)  //
      const noexcept {
    return std::hash<
        USERVER_NAMESPACE::storages::postgres::TimePoint::duration::rep>{}(
        v.GetUnderlying().time_since_epoch().count());
  }
};
