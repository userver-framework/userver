#pragma once

#include <chrono>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages::postgres::io {

namespace detail {

/// Internal structure for representing PostgreSQL interval type
struct Interval {
  using DurationType = std::chrono::microseconds;

  Integer months = 0;
  Integer days = 0;
  Bigint microseconds = 0;

  constexpr Interval() = default;
  constexpr Interval(Integer months, Integer days, Bigint microseconds)
      : months{months}, days{days}, microseconds{microseconds} {}
  constexpr explicit Interval(DurationType ms) : microseconds{ms.count()} {}

  constexpr bool operator==(const Interval& rhs) const {
    return months == rhs.months && days == rhs.days &&
           microseconds == rhs.microseconds;
  }
  constexpr bool operator!=(const Interval& rhs) const {
    return !(*this == rhs);
  }

  constexpr DurationType GetDuration() const {
    if (months != 0) throw UnsupportedInterval{};
    return std::chrono::duration_cast<DurationType>(
        std::chrono::microseconds{microseconds} +
        std::chrono::hours{days * 24});
  }
};

}  // namespace detail

template <>
struct BufferParser<detail::Interval, DataFormat::kBinaryDataFormat>
    : detail::BufferParserBase<detail::Interval> {
  using BaseType = detail::BufferParserBase<detail::Interval>;
  using ValueType = typename BaseType::ValueType;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    ValueType tmp;
    std::size_t offset = 0;
    ReadBinary(buffer.GetSubBuffer(offset, sizeof(Bigint),
                                   BufferCategory::kPlainBuffer),
               tmp.microseconds);
    offset += sizeof(Bigint);
    ReadBinary(buffer.GetSubBuffer(offset, sizeof(Integer),
                                   BufferCategory::kPlainBuffer),
               tmp.days);
    offset += sizeof(Integer);
    ReadBinary(buffer.GetSubBuffer(offset, sizeof(Integer),
                                   BufferCategory::kPlainBuffer),
               tmp.months);

    std::swap(this->value, tmp);
  }
};

template <>
struct BufferFormatter<detail::Interval, DataFormat::kBinaryDataFormat>
    : detail::BufferFormatterBase<detail::Interval> {
  using BaseType = detail::BufferFormatterBase<detail::Interval>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WriteBinary(types, buffer, this->value.microseconds);
    WriteBinary(types, buffer, this->value.days);
    WriteBinary(types, buffer, this->value.months);
  }
};

template <>
struct CppToSystemPg<detail::Interval>
    : PredefinedOid<PredefinedOids::kInterval> {};

}  // namespace storages::postgres::io
