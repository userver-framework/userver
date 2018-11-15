#pragma once

#include <cstring>
#include <string>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages {
namespace postgres {
namespace io {

//@{
/** @name const char* formatting */
template <>
struct BufferFormatter<const char*, DataFormat::kTextDataFormat> {
  const char* value;

  explicit BufferFormatter(const char* val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    auto sz = std::strlen(value);
    WriteN(buf, value, sz);
  }

  template <typename Buffer>
  static void WriteN(Buffer& buf, const char* c, std::size_t n) {
    buf.reserve(buf.size() + n);
    std::copy(c, c + n, std::back_inserter(buf));
    if (n == 0 || buf.back() != '\0') buf.push_back('\0');
  }
};

template <>
struct BufferFormatter<const char*, DataFormat::kBinaryDataFormat> {
  const char* value;

  explicit BufferFormatter(const char* val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    auto sz = std::strlen(value);
    WriteN(buf, value, sz);
  }

  template <typename Buffer>
  static void WriteN(Buffer& buf, const char* c, std::size_t n) {
    // Don't copy zero-terminator in binary mode
    while (n > 0 && c[n - 1] == '\0') {
      --n;
    }
    buf.reserve(buf.size() + n);
    std::copy(c, c + n, std::back_inserter(buf));
  }
};
//@}

//@{
/** @name char[N] formatting */
template <std::size_t N, DataFormat F>
struct BufferFormatter<char[N], F> {
  using CharFormatter = BufferFormatter<const char*, F>;
  const char* value;

  explicit BufferFormatter(const char* val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    CharFormatter::WriteN(buf, value, N);
  }
};

//@}

//@{
/** @name std::string I/O */
template <DataFormat F>
struct BufferFormatter<std::string, F> {
  using CharFormatter = BufferFormatter<const char*, F>;
  const std::string& value;

  explicit BufferFormatter(const std::string& val) : value{val} {}
  template <typename Buffer>
  void operator()(Buffer& buf) const {
    CharFormatter::WriteN(buf, value.data(), value.size());
  }
};

template <>
struct BufferParser<std::string, DataFormat::kTextDataFormat> {
  std::string& value;

  explicit BufferParser(std::string& val) : value{val} {}

  void operator()(const FieldBuffer& buffer);
};

template <>
struct BufferParser<std::string, DataFormat::kBinaryDataFormat> {
  std::string& value;

  explicit BufferParser(std::string& val) : value{val} {}

  void operator()(const FieldBuffer& buffer);
};
//@}

//@{
/** @name char I/O */
template <>
struct BufferFormatter<char, DataFormat::kBinaryDataFormat> {
  char value;

  explicit BufferFormatter(char val) : value{val} {}
  template <typename Buffer>
  void operator()(Buffer& buf) const {
    buf.push_back(value);
  }
};

template <>
struct BufferFormatter<char, DataFormat::kTextDataFormat>
    : BufferFormatter<char, DataFormat::kBinaryDataFormat> {
  using BaseType = BufferFormatter<char, DataFormat::kBinaryDataFormat>;
  using BaseType::BaseType;
};

template <>
struct BufferParser<char, DataFormat::kBinaryDataFormat> {
  char& value;

  explicit BufferParser(char& val) : value{val} {}

  void operator()(const FieldBuffer& buffer) {
    if (buffer.length != 1) {
      throw InvalidInputBufferSize{buffer.length, "for type char"};
    }
    value = *buffer.buffer;
  }
};

template <>
struct BufferParser<char, DataFormat::kTextDataFormat>
    : BufferParser<char, DataFormat::kBinaryDataFormat> {
  using BaseType = BufferParser<char, DataFormat::kBinaryDataFormat>;
  using BaseType::BaseType;
};
//@}

// TODO string_view

//@{
/** @name C++ to PostgreSQL mapping for string types */
template <>
struct CppToPg<const char*>
    : detail::CppToPgPredefined<const char*, PredefinedOids::kText> {};
template <std::size_t N>
struct CppToPg<char[N]>
    : detail::CppToPgPredefined<char[N], PredefinedOids::kText> {};
template <>
struct CppToPg<std::string>
    : detail::CppToPgPredefined<std::string, PredefinedOids::kText> {};
template <>
struct CppToPg<char> : detail::CppToPgPredefined<char, PredefinedOids::kChar> {
};
//@}

}  // namespace io
}  // namespace postgres
}  // namespace storages
