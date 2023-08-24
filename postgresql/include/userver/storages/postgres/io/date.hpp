#pragma once

/// @file userver/storages/postgres/io/date.hpp
/// @brief utils::datetime::Date I/O support
/// @ingroup userver_postgres_parse_and_format

#include <limits>

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/utils/datetime/date.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// Corresponds to DATE
using Date = USERVER_NAMESPACE::utils::datetime::Date;

/// Postgres epoch date (2000-01-01)
Date PostgresEpochDate();

/// Constant equivalent to PostgreSQL 'infinity'::date, a date that is later
/// than all other dates.
/// https://www.postgresql.org/docs/current/datatype-datetime.html#DATATYPE-DATETIME-SPECIAL-TABLE
inline constexpr Date kDatePositiveInfinity = Date::SysDays::max();

/// Constant equivalent to PostgreSQL '-infinity'::date, a date that is earlier
/// than all other dates.
/// https://www.postgresql.org/docs/current/datatype-datetime.html#DATATYPE-DATETIME-SPECIAL-TABLE
inline constexpr Date kDateNegativeInfinity = Date::SysDays::min();

namespace io {

/// @brief Binary formatter for utils::datetime::Date
template <>
struct BufferFormatter<Date> {
  const Date value;

  explicit BufferFormatter(Date value) : value{value} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) {
    static const auto kPgEpoch = PostgresEpochDate();
    if (value == kDatePositiveInfinity) {
      WriteBuffer(types, buffer, std::numeric_limits<Integer>::max());
    } else if (value == kDateNegativeInfinity) {
      WriteBuffer(types, buffer, std::numeric_limits<Integer>::min());
    } else {
      auto pg_days = static_cast<Integer>(
          (value.GetSysDays() - kPgEpoch.GetSysDays()).count());
      WriteBuffer(types, buffer, pg_days);
    }
  }
};

/// @brief Binary parser for utils::datetime::Date
template <>
struct BufferParser<Date> : detail::BufferParserBase<Date> {
  using BaseType = detail::BufferParserBase<Date>;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    static const auto kPgEpoch = PostgresEpochDate();
    Integer pg_days{0};
    ReadBuffer(buffer, pg_days);
    if (pg_days == std::numeric_limits<Integer>::max()) {
      this->value = kDatePositiveInfinity;
    } else if (pg_days == std::numeric_limits<Integer>::min()) {
      this->value = kDateNegativeInfinity;
    } else {
      this->value = kPgEpoch.GetSysDays() + Date::Days{pg_days};
    }
  }
};

template <>
struct CppToSystemPg<Date> : PredefinedOid<PredefinedOids::kDate> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
