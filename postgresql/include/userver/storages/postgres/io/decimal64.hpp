#pragma once

/// @file userver/storages/postgres/io/decimal64.hpp
/// @brief decimal64::Decimal I/O support
/// @ingroup userver_postgres_parse_and_format

#include <userver/decimal64/decimal64.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/numeric_data.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

template <int Prec, typename RoundPolicy>
struct BufferFormatter<decimal64::Decimal<Prec, RoundPolicy>>
    : detail::BufferFormatterBase<decimal64::Decimal<Prec, RoundPolicy>> {
  using BaseType =
      detail::BufferFormatterBase<decimal64::Decimal<Prec, RoundPolicy>>;
  using BaseType::BaseType;
  using ValueType = typename BaseType::ValueType;

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buffer) const {
    auto bin_str =
        detail::Int64ToNumericBuffer({this->value.AsUnbiased(), Prec});
    buffer.reserve(buffer.size() + bin_str.size());
    std::copy(bin_str.begin(), bin_str.end(), std::back_inserter(buffer));
  }
};

template <int Prec, typename RoundPolicy>
struct BufferParser<decimal64::Decimal<Prec, RoundPolicy>>
    : detail::BufferParserBase<decimal64::Decimal<Prec, RoundPolicy>> {
  using BaseType =
      detail::BufferParserBase<decimal64::Decimal<Prec, RoundPolicy>>;
  using BaseType::BaseType;
  using ValueType = typename BaseType::ValueType;

  void operator()(const FieldBuffer& buffer) {
    auto rep = detail::NumericBufferToInt64(buffer);
    this->value = ValueType::FromBiased(rep.value, rep.fractional_digit_count);
  }
};

template <int Prec, typename RoundPolicy>
struct CppToSystemPg<decimal64::Decimal<Prec, RoundPolicy>>
    : PredefinedOid<PredefinedOids::kNumeric> {};
}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
