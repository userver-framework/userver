#pragma once

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/stream_text_parser.hpp>
#include <storages/postgres/io/type_mapping.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

namespace storages {
namespace postgres {

template <std::size_t Precision = 50>
using ArbitraryPrecision = boost::multiprecision::number<
    boost::multiprecision::cpp_dec_float<Precision>>;

using Numeric = ArbitraryPrecision<50>;

namespace io {

namespace detail {

std::string NumericBufferToString(const FieldBuffer& buffer);

}  // namespace detail

template <std::size_t Precision>
struct BufferFormatter<ArbitraryPrecision<Precision>,
                       DataFormat::kTextDataFormat> {
  using Value = ArbitraryPrecision<Precision>;
  const Value& value;

  BufferFormatter(const Value& val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    using sink_type = boost::iostreams::back_insert_device<Buffer>;
    using stream_type = boost::iostreams::stream<sink_type>;

    {
      sink_type sink{buf};
      stream_type os{sink};
      os << std::setprecision(std::numeric_limits<Value>::digits10) << value;
    }
    if (buf.empty() || buf.back() != '\0') buf.push_back('\0');
  }
};

template <std::size_t Precision>
struct BufferParser<ArbitraryPrecision<Precision>,
                    DataFormat::kBinaryDataFormat>
    : detail::BufferParserBase<ArbitraryPrecision<Precision>> {
  using BaseType = detail::BufferParserBase<ArbitraryPrecision<Precision>>;
  using BaseType::BaseType;
  using NumberType = boost::multiprecision::cpp_dec_float<Precision>;

  void operator()(const FieldBuffer& buffer) {
    auto str = detail::NumericBufferToString(buffer);
    // cpp_dec_float provides const char* constructor, where it parses the
    // contents to internal buffers. no std::string constructor is provided.
    this->value = NumberType{str.c_str()};
  }
};

template <std::size_t Precision>
struct CppToSystemPg<ArbitraryPrecision<Precision>>
    : PredefinedOid<PredefinedOids::kNumeric> {};

}  // namespace io
}  // namespace postgres
}  // namespace storages
