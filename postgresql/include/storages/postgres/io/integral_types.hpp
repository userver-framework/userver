#pragma once

#include <boost/endian/arithmetic.hpp>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

#include <storages/postgres/io/stream_text_formatter.hpp>
#include <storages/postgres/io/stream_text_parser.hpp>

namespace storages {
namespace postgres {
namespace io {

namespace detail {

template <std::size_t Size>
struct IntegralType;

template <>
struct IntegralType<2> {
  using type = Smallint;
};

template <>
struct IntegralType<4> {
  using type = Integer;
};

template <>
struct IntegralType<8> {
  using type = Bigint;
};

template <std::size_t Size>
struct IntegralBySizeParser {
  using IntType = typename IntegralType<Size>::type;
  constexpr static std::size_t size = Size;

  static IntType ParseBuffer(const FieldBuffer& buf) {
    const IntType* p = reinterpret_cast<const IntType*>(buf.buffer);
    return boost::endian::big_to_native(*p);
  }
};

template <typename T>
struct IntegralBinaryParser {
  T& value;
  explicit IntegralBinaryParser(T& val) : value{val} {}
  void operator()(const FieldBuffer& buf) {
    switch (buf.length) {
      case 2:
        value = IntegralBySizeParser<2>::ParseBuffer(buf);
        break;
      case 4:
        value = IntegralBySizeParser<4>::ParseBuffer(buf);
        break;
      case 8:
        value = IntegralBySizeParser<8>::ParseBuffer(buf);
        break;
      default:
        throw InvalidInputBufferSize{buf.length, "for an integral value type"};
    }
  }
};

template <typename T>
struct IntegralBinaryFormatter {
  static constexpr std::size_t size = sizeof(T);
  T value;

  explicit IntegralBinaryFormatter(T val) : value{val} {}
  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    buf.reserve(buf.size() + size);
    auto tmp = boost::endian::native_to_big(value);
    const char* p = reinterpret_cast<char const*>(&tmp);
    const char* e = p + size;
    std::copy(p, e, std::back_inserter(buf));
  }

  /// Write the value to char buffer, the buffer MUST be already resized
  template <typename Iterator>
  void operator()(Iterator buffer) const {
    auto tmp = boost::endian::native_to_big(value);
    const char* p = reinterpret_cast<char const*>(&tmp);
    const char* e = p + size;
    std::copy(p, e, buffer);
  }
};

}  // namespace detail

//@{
/** @name 2 byte integer */
template <>
struct BufferParser<Smallint, DataFormat::kBinaryDataFormat>
    : detail::IntegralBinaryParser<Smallint> {
  explicit BufferParser(Smallint& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Smallint, DataFormat::kBinaryDataFormat>
    : detail::IntegralBinaryFormatter<Smallint> {
  explicit BufferFormatter(Smallint val) : IntegralBinaryFormatter(val) {}
};
//@}

//@{
/** @name 4 byte integer */
template <>
struct BufferParser<Integer, DataFormat::kBinaryDataFormat>
    : detail::IntegralBinaryParser<Integer> {
  explicit BufferParser(Integer& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Integer, DataFormat::kBinaryDataFormat>
    : detail::IntegralBinaryFormatter<Integer> {
  explicit BufferFormatter(Integer val) : IntegralBinaryFormatter(val) {}
};
//@}

//@{
/** @name 8 byte integer */
template <>
struct BufferParser<Bigint, DataFormat::kBinaryDataFormat>
    : detail::IntegralBinaryParser<Bigint> {
  explicit BufferParser(Bigint& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Bigint, DataFormat::kBinaryDataFormat>
    : detail::IntegralBinaryFormatter<Bigint> {
  explicit BufferFormatter(Bigint val) : IntegralBinaryFormatter(val) {}
};
//@}

//@{
/** @name boolean */
template <>
struct BufferParser<bool, DataFormat::kBinaryDataFormat> {
  bool& value;
  explicit BufferParser(bool& val) : value{val} {}
  void operator()(const FieldBuffer& buf) {
    if (buf.length != 1) {
      throw InvalidInputBufferSize{buf.length, "for boolean type"};
    }
    value = *buf.buffer != 0;
  }
};

template <>
struct BufferParser<bool, DataFormat::kTextDataFormat> {
  bool& value;
  explicit BufferParser(bool& val) : value{val} {}
  void operator()(const FieldBuffer& buf);
};

template <>
struct BufferFormatter<bool, DataFormat::kBinaryDataFormat> {
  bool value;
  explicit BufferFormatter(bool val) : value(val) {}
  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    buf.push_back(value ? 1 : 0);
  }
};

template <>
struct BufferFormatter<bool, DataFormat::kTextDataFormat> {
  bool value;
  explicit BufferFormatter(bool val) : value(val) {}
  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    buf.push_back(value ? 't' : 'f');
  }
};
//@}

//@{
/** @name C++ to PostgreSQL mapping for integral types */
template <>
struct CppToSystemPg<Smallint> : PredefinedOid<PredefinedOids::kInt2> {};
template <>
struct CppToSystemPg<Integer> : PredefinedOid<PredefinedOids::kInt4> {};
template <>
struct CppToSystemPg<Bigint> : PredefinedOid<PredefinedOids::kInt8> {};
template <>
struct CppToSystemPg<bool> : PredefinedOid<PredefinedOids::kBoolean> {};
//@}

}  // namespace io
}  // namespace postgres
}  // namespace storages
