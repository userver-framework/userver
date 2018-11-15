#pragma once

#include <time_zone.h>
#include <chrono>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

#include <utils/demangle.hpp>

namespace storages {
namespace postgres {

using ClockType = std::chrono::system_clock;
using TimePoint = ClockType::time_point;

TimePoint PostgresEpoch();

/**
 * @page PostrgeSQL timestamp support
 *
 * The driver provides mapping from C++ std::chrono::time_point template type to
 * Postgres timestamp (without time zone) data type.
 *
 * The value is formatted/parsed using UTC timezone.
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
 * // argument, defaults to UTC
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
  return {val, cctz::utc_time_zone()};
}

template <typename Duration>
detail::TimestampTz<const std::chrono::time_point<ClockType, Duration>>
TimestampTz(const std::chrono::time_point<ClockType, Duration>& val) {
  return {val, cctz::utc_time_zone()};
}

namespace io {

/// @brief Text formatter for timestamptz.
template <typename TimePointType>
struct BufferFormatter<postgres::detail::TimestampTz<TimePointType>,
                       DataFormat::kTextDataFormat> {
  using ValueType = postgres::detail::TimestampTz<TimePointType>;
  ValueType value;

  explicit BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    static const std::string format = "%Y-%m-%d %H:%M:%E*S%Ez";
    auto str = cctz::format(format, value.value, value.tz);
    buf.reserve(buf.size() + str.size() + 1);
    std::copy(str.begin(), str.end(), std::back_inserter(buf));
    buf.push_back('\0');
  }
};

/// @brief Binary formatter for timestamptz.
/// PostgreSQL expects timestamptz in UTC when in binary format.
template <typename TimePointType>
struct BufferFormatter<postgres::detail::TimestampTz<TimePointType>,
                       DataFormat::kBinaryDataFormat> {
  using ValueType = postgres::detail::TimestampTz<TimePointType>;
  ValueType value;

  explicit BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    // TODO More robust method for detecting local timezone offset. ICU?
    static const auto tz = cctz::local_time_zone();
    auto lookup = tz.lookup(value.value);
    auto val = value.value - std::chrono::seconds{lookup.offset};
    WriteBuffer<DataFormat::kBinaryDataFormat>(buf, val);
  }
};

/// @brief Text parser for timestamptz.
template <typename Duration>
struct BufferParser<
    postgres::detail::TimestampTz<std::chrono::time_point<ClockType, Duration>>,
    DataFormat::kTextDataFormat> {
  using ValueType = postgres::detail::TimestampTz<
      std::chrono::time_point<ClockType, Duration>>;
  ValueType value;

  explicit BufferParser(ValueType&& val) : value{std::move(val)} {}

  void operator()(const FieldBuffer& buffer) {
    static const std::string format = "%Y-%m-%d %H:%M:%E*S%Ez";
    std::string timestr{buffer.buffer, buffer.length};
    typename ValueType::TimePointType tmp;
    if (cctz::parse(format, timestr, value.tz, &tmp)) {
      std::swap(tmp, value.value);
    } else {
      std::string b{buffer.buffer, buffer.length};
      throw TextParseFailure{::utils::GetTypeName<ValueType>(), b};
    }
  }
};

/// @brief Binary parser for timestamptz.
///
template <typename Duration>
struct BufferParser<
    postgres::detail::TimestampTz<std::chrono::time_point<ClockType, Duration>>,
    DataFormat::kBinaryDataFormat> {
  using ValueType = postgres::detail::TimestampTz<
      std::chrono::time_point<ClockType, Duration>>;
  ValueType value;

  explicit BufferParser(ValueType&& val) : value{std::move(val)} {}

  void operator()(const FieldBuffer& buffer) {
    // TODO More robust method for detecting local timezone offset. ICU?
    static const auto tz = cctz::local_time_zone();
    auto lookup = tz.lookup(value.value);
    typename ValueType::TimePointType tmp;
    ReadBuffer<DataFormat::kBinaryDataFormat>(buffer, tmp);
    value.value = tmp + std::chrono::seconds{lookup.offset};
  }
};

/// @brief Text formatter for std::chrono::time_point.
template <typename Duration>
struct BufferFormatter<std::chrono::time_point<ClockType, Duration>,
                       DataFormat::kTextDataFormat> {
  using ValueType = std::chrono::time_point<ClockType, Duration>;

  const ValueType& value;

  explicit BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    static const std::string format = "%Y-%m-%d %H:%M:%E*S";
    static const auto tz = cctz::local_time_zone();
    auto str = cctz::format(format, value, tz);
    buf.reserve(buf.size() + str.size() + 1);
    std::copy(str.begin(), str.end(), std::back_inserter(buf));
    buf.push_back('\0');
  }
};

/// @brief Binary formatter for std::chrono::time_point.
template <typename Duration>
struct BufferFormatter<std::chrono::time_point<ClockType, Duration>,
                       DataFormat::kBinaryDataFormat> {
  using ValueType = std::chrono::time_point<ClockType, Duration>;

  const ValueType& value;

  explicit BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    static const ValueType pg_epoch = PostgresEpoch();
    auto tmp =
        std::chrono::duration_cast<std::chrono::microseconds>(value - pg_epoch)
            .count();
    WriteBuffer<DataFormat::kBinaryDataFormat>(buf, tmp);
  }
};

/// @brief Text parser for std::chrono::time_point.
template <typename Duration>
struct BufferParser<std::chrono::time_point<ClockType, Duration>,
                    DataFormat::kTextDataFormat> {
  using ValueType = std::chrono::time_point<ClockType, Duration>;

  ValueType& value;
  explicit BufferParser(ValueType& val) : value{val} {}

  void operator()(const FieldBuffer& buffer) {
    static const std::string format = "%Y-%m-%d %H:%M:%E*S";
    static const auto tz = cctz::local_time_zone();
    std::string timestr{buffer.buffer, buffer.length};
    ValueType tmp;
    if (cctz::parse(format, timestr, tz, &tmp)) {
      std::swap(tmp, value);
    } else {
      std::string b{buffer.buffer, buffer.length};
      throw TextParseFailure{::utils::GetTypeName<ValueType>(), b};
    }
  }
};

/// @brief Binary parser for std::chrono::time_point.
template <typename Duration>
struct BufferParser<std::chrono::time_point<ClockType, Duration>,
                    DataFormat::kBinaryDataFormat> {
  using ValueType = std::chrono::time_point<ClockType, Duration>;

  ValueType& value;
  explicit BufferParser(ValueType& val) : value{val} {}

  void operator()(const FieldBuffer& buffer) {
    static const ValueType pg_epoch = PostgresEpoch();
    Bigint usec{0};
    ReadBuffer<DataFormat::kBinaryDataFormat>(buffer, usec);
    ValueType tmp = pg_epoch + std::chrono::microseconds{usec};
    std::swap(tmp, value);
  }
};

template <typename TimePointType>
struct CppToPg<postgres::detail::TimestampTz<TimePointType>>
    : detail::CppToPgPredefined<postgres::detail::TimestampTz<TimePointType>,
                                PredefinedOids::kTimestamptz> {};

template <typename Duration>
struct CppToPg<std::chrono::time_point<ClockType, Duration>>
    : detail::CppToPgPredefined<std::chrono::time_point<ClockType, Duration>,
                                PredefinedOids::kTimestamp> {};

}  // namespace io
}  // namespace postgres
}  // namespace storages
