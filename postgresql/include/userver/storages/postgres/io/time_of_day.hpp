#pragma once

/// @file userver/storages/postgres/io/time_of_day.hpp
/// @brief utils::datetime::TimeOfDay I/O support
/// @ingroup userver_postgres_parse_and_format

#include <userver/utils/time_of_day.hpp>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

/// @brief Binary formatter for utils::datetime::TimeOfDay mapped to postgres
/// time
/// This datatype is time-zone agnostic, it should't be mixed with timetz type
/// or sudden TZ adjustments will be applied.
template <typename Duration>
struct BufferFormatter<USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>>
    : detail::BufferFormatterBase<
          USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>> {
  using BaseType = detail::BufferFormatterBase<
      USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    Bigint usec = std::chrono::duration_cast<std::chrono::microseconds>(
                      this->value.SinceMidnight())
                      .count();
    io::WriteBuffer(types, buffer, usec);
  }
};

/// @brief Binary parser for utils::datetime::TimeOfDay mapped to postgres time
template <typename Duration>
struct BufferParser<USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>>
    : detail::BufferParserBase<
          USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>> {
  using BaseType = detail::BufferParserBase<
      USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>>;
  using ValueType = typename BaseType::ValueType;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    Bigint usec{0};
    io::ReadBuffer(buffer, usec);
    this->value = ValueType{
        std::chrono::duration_cast<Duration>(std::chrono::microseconds{usec})};
  }
};

template <typename Duration>
struct CppToSystemPg<USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>>
    : PredefinedOid<PredefinedOids::kTime> {};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
