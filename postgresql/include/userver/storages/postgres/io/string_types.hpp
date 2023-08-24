#pragma once

/// @file userver/storages/postgres/io/string_types.hpp
/// @brief Strings I/O support
/// @ingroup userver_postgres_parse_and_format

#include <cstring>
#include <string>
#include <string_view>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

//@{
/** @name const char* formatting */
template <>
struct BufferFormatter<const char*> {
  const char* value;

  explicit BufferFormatter(const char* val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
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
template <std::size_t N>
struct BufferFormatter<char[N]> {
  using CharFormatter = BufferFormatter<const char*>;
  const char* value;

  explicit BufferFormatter(const char* val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    CharFormatter::WriteN(buf, value, N);
  }
};

//@}

//@{
/** @name std::string I/O */
template <>
struct BufferFormatter<std::string> {
  using CharFormatter = BufferFormatter<const char*>;
  const std::string& value;

  explicit BufferFormatter(const std::string& val) : value{val} {}
  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    CharFormatter::WriteN(buf, value.data(), value.size());
  }
};

template <>
struct BufferParser<std::string> {
  std::string& value;

  explicit BufferParser(std::string& val) : value{val} {}

  void operator()(const FieldBuffer& buffer);
};
//@}

//@{
/** @name string_view I/O */
template <>
struct BufferFormatter<std::string_view>
    : detail::BufferFormatterBase<std::string_view> {
  using BaseType = detail::BufferFormatterBase<std::string_view>;
  using CharFormatter = BufferFormatter<const char*>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buffer) const {
    CharFormatter::WriteN(buffer, value.data(), value.size());
  }
};

template <>
struct BufferParser<std::string_view>
    : detail::BufferParserBase<std::string_view> {
  using BaseType = detail::BufferParserBase<std::string_view>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    using std::swap;
    ValueType tmp{reinterpret_cast<const char*>(buffer.buffer), buffer.length};
    swap(tmp, value);
  }
};
//@}

//@{
/** @name char I/O */
template <>
struct BufferFormatter<char> {
  char value;

  explicit BufferFormatter(char val) : value{val} {}
  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    buf.push_back(value);
  }
};

template <>
struct BufferParser<char> {
  char& value;

  explicit BufferParser(char& val) : value{val} {}

  void operator()(const FieldBuffer& buffer) {
    if (buffer.length != 1) {
      throw InvalidInputBufferSize{buffer.length, "for type char"};
    }
    value = *buffer.buffer;
  }
};
//@}

//@{
/** @name C++ to PostgreSQL mapping for string types */
template <>
struct CppToSystemPg<const char*> : PredefinedOid<PredefinedOids::kText> {};
template <std::size_t N>
struct CppToSystemPg<char[N]> : PredefinedOid<PredefinedOids::kText> {};
template <>
struct CppToSystemPg<std::string> : PredefinedOid<PredefinedOids::kText> {};
template <>
struct CppToSystemPg<std::string_view> : PredefinedOid<PredefinedOids::kText> {
};
template <>
struct CppToSystemPg<char> : PredefinedOid<PredefinedOids::kChar> {};
//@}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
