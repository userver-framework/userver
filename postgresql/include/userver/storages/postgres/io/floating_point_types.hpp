#pragma once

/// @file userver/storages/postgres/io/floating_point_types.hpp
/// @brief Floating point I/O support
/// @ingroup userver_postgres_parse_and_format

#include <cstring>

#include <boost/endian/arithmetic.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

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
    const IntType tmp = IntParser::ParseBuffer(buf);
    FloatType float_value{};
    std::memcpy(&float_value, &tmp, Size);
    return float_value;
  }
};

template <typename T>
struct FloatingPointBinaryParser : BufferParserBase<T> {
  using BaseType = BufferParserBase<T>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buf) {
    switch (buf.length) {
      case 4:
        this->value = FloatingPointBySizeParser<4>::ParseBuffer(buf);
        break;
      case 8:
        this->value = FloatingPointBySizeParser<8>::ParseBuffer(buf);
        break;
      default:
        throw InvalidInputBufferSize{buf.length,
                                     "for a floating point value type"};
    }
  }
};

template <typename T>
struct FloatingPointBinaryFormatter {
  static constexpr std::size_t size = sizeof(T);
  T value;

  explicit FloatingPointBinaryFormatter(T val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    using IntType = typename IntegralType<size>::type;
    IntType int_value{};
    std::memcpy(&int_value, &value, size);
    IntegralBinaryFormatter<IntType>{int_value}(types, buf);
  }
};
}  // namespace detail

//@{
/** @name 4 byte floating point */
template <>
struct BufferParser<float> : detail::FloatingPointBinaryParser<float> {
  explicit BufferParser(float& val) : FloatingPointBinaryParser(val) {}
};
template <>
struct BufferFormatter<float> : detail::FloatingPointBinaryFormatter<float> {
  explicit BufferFormatter(float val) : FloatingPointBinaryFormatter(val) {}
};
//@}

//@{
/** @name 8 byte floating point */
template <>
struct BufferParser<double> : detail::FloatingPointBinaryParser<double> {
  explicit BufferParser(double& val) : FloatingPointBinaryParser(val) {}
};
template <>
struct BufferFormatter<double> : detail::FloatingPointBinaryFormatter<double> {
  explicit BufferFormatter(double val) : FloatingPointBinaryFormatter(val) {}
};
//@}

//@{
/** @name C++ to PostgreSQL mapping for floating point types */
template <>
struct CppToSystemPg<float> : PredefinedOid<PredefinedOids::kFloat4> {};
template <>
struct CppToSystemPg<double> : PredefinedOid<PredefinedOids::kFloat8> {};
//@}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
