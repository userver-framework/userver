#pragma once

#include <storages/postgres/io/stream_text_parser.hpp>
#include <storages/postgres/io/traits.hpp>
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

template <std::size_t Precision>
struct BufferFormatter<ArbitraryPrecision<Precision>,
                       DataFormat::kTextDataFormat> {
  using Value = ArbitraryPrecision<Precision>;
  const Value& value;

  BufferFormatter(const Value& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
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
struct CppToPg<ArbitraryPrecision<Precision>>
    : detail::CppToPgPredefined<ArbitraryPrecision<Precision>,
                                PredefinedOids::kNumeric> {};

}  // namespace io
}  // namespace postgres
}  // namespace storages
