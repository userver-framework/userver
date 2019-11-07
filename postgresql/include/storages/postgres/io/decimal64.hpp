#pragma once

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/numeric_data.hpp>
#include <storages/postgres/io/type_mapping.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <decimal64/decimal64.hpp>

namespace storages::postgres::io {

template <int Prec, typename RoundPolicy>
struct BufferFormatter<decimal64::decimal<Prec, RoundPolicy>,
                       DataFormat::kBinaryDataFormat>
    : detail::BufferFormatterBase<decimal64::decimal<Prec, RoundPolicy>> {
  using BaseType =
      detail::BufferFormatterBase<decimal64::decimal<Prec, RoundPolicy>>;
  using BaseType::BaseType;
  using ValueType = typename BaseType::ValueType;

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buffer) const {
    auto bin_str =
        detail::Int64ToNumericBuffer({this->value.getUnbiased(), Prec});
    buffer.reserve(buffer.size() + bin_str.size());
    std::copy(bin_str.begin(), bin_str.end(), std::back_inserter(buffer));
  }
};

template <int Prec, typename RoundPolicy>
struct BufferParser<decimal64::decimal<Prec, RoundPolicy>,
                    DataFormat::kBinaryDataFormat>
    : detail::BufferParserBase<decimal64::decimal<Prec, RoundPolicy>> {
  using BaseType =
      detail::BufferParserBase<decimal64::decimal<Prec, RoundPolicy>>;
  using BaseType::BaseType;
  using ValueType = typename BaseType::ValueType;

  void operator()(const FieldBuffer& buffer) {
    auto rep = detail::NumericBufferToInt64(buffer);
    this->value = ValueType{rep.value, decimal64::dec_utils<RoundPolicy>::pow10(
                                           rep.fractional_digit_count)};
  }
};

template <int Prec, typename RoundPolicy>
struct CppToSystemPg<decimal64::decimal<Prec, RoundPolicy>>
    : PredefinedOid<PredefinedOids::kNumeric> {};
}  // namespace storages::postgres::io
