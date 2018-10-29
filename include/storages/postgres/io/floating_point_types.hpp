#pragma once

#include <boost/endian/arithmetic.hpp>
#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages {
namespace postgres {
namespace io {

namespace detail {

template <std::size_t>
struct FloatingPointType;

template <>
struct FloatingPointType<4> {
  using type = float;
};

template <>
struct FloatingPointType<8> {
  using type = double;
};

template <std::size_t Size>
struct FloatingPointBySizeParser {
  using FloatType = typename FloatingPointType<Size>::type;
  using IntType = typename IntegralType<Size>::type;
  using IntParser = IntegralBySizeParser<Size>;

  static FloatType ParseBuffer(const FieldBuffer& buf) {
    IntType tmp = IntParser::ParseBuffer(buf);
    return reinterpret_cast<const FloatType&>(tmp);
  }
};

template <typename T>
struct FloatingPointBinaryParser {
  T& value;

  explicit FloatingPointBinaryParser(T& val) : value{val} {}
  void operator()(const FieldBuffer& buf) {
    switch (buf.length) {
      case 4:
        value = FloatingPointBySizeParser<4>::ParseBuffer(buf);
        break;
      case 8:
        value = FloatingPointBySizeParser<8>::ParseBuffer(buf);
        break;
      default:
        // TODO Throw logic(?) exception here
        break;
    }
  }
};

template <typename T>
struct FloatingPointBinaryFormatter {
  static constexpr std::size_t size = sizeof(T);
  T value;

  explicit FloatingPointBinaryFormatter(T val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    using IntType = typename IntegralType<size>::type;
    IntegralBinaryFormatter<IntType>(reinterpret_cast<const IntType&>(value))(
        buf);
  }
};
}  // namespace detail

//@{
/** @name 4 byte floating point */
template <>
struct BufferParser<float, DataFormat::kBinaryDataFormat>
    : detail::FloatingPointBinaryParser<float> {
  explicit BufferParser(float& val) : FloatingPointBinaryParser(val) {}
};
template <>
struct BufferFormatter<float, DataFormat::kBinaryDataFormat>
    : detail::FloatingPointBinaryFormatter<float> {
  explicit BufferFormatter(float val) : FloatingPointBinaryFormatter(val) {}
};
//@}

//@{
/** @name 8 byte floating point */
template <>
struct BufferParser<double, DataFormat::kBinaryDataFormat>
    : detail::FloatingPointBinaryParser<double> {
  explicit BufferParser(double& val) : FloatingPointBinaryParser(val) {}
};
template <>
struct BufferFormatter<double, DataFormat::kBinaryDataFormat>
    : detail::FloatingPointBinaryFormatter<double> {
  explicit BufferFormatter(double val) : FloatingPointBinaryFormatter(val) {}
};
//@}

//@{
/** @name C++ to PostgreSQL mapping for floating point types */
template <>
struct CppToPg<float> : detail::CppToPgPredefined<PredefinedOids::kFloat4> {};
template <>
struct CppToPg<double> : detail::CppToPgPredefined<PredefinedOids::kFloat8> {};
//@}

}  // namespace io
}  // namespace postgres
}  // namespace storages
