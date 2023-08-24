#pragma once

/// @file userver/storages/postgres/io/boost_multiprecision.hpp
/// @brief boost::multiprecision I/O support
/// @ingroup userver_postgres_parse_and_format

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/numeric_data.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

#include <boost/multiprecision/cpp_dec_float.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

template <unsigned Precision = 50>
using MultiPrecision = boost::multiprecision::number<
    boost::multiprecision::cpp_dec_float<Precision>>;

namespace io {

template <unsigned Precision>
struct BufferFormatter<MultiPrecision<Precision>> {
  using Value = MultiPrecision<Precision>;
  const Value& value;

  BufferFormatter(const Value& val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    auto str_rep = value.str(std::numeric_limits<Value>::max_digits10,
                             std::ios_base::fixed);
    auto bin_str = detail::StringToNumericBuffer(str_rep);
    buf.reserve(buf.size() + bin_str.size());
    std::copy(bin_str.begin(), bin_str.end(), std::back_inserter(buf));
  }
};

template <unsigned Precision>
struct BufferParser<MultiPrecision<Precision>>
    : detail::BufferParserBase<MultiPrecision<Precision>> {
  using BaseType = detail::BufferParserBase<MultiPrecision<Precision>>;
  using BaseType::BaseType;
  using NumberType = boost::multiprecision::cpp_dec_float<Precision>;

  void operator()(const FieldBuffer& buffer) {
    auto str = detail::NumericBufferToString(buffer);
    // cpp_dec_float provides const char* constructor, where it parses the
    // contents to internal buffers. no std::string constructor is provided.
    this->value = NumberType{str.c_str()};
  }
};

template <unsigned Precision>
struct CppToSystemPg<MultiPrecision<Precision>>
    : PredefinedOid<PredefinedOids::kNumeric> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
