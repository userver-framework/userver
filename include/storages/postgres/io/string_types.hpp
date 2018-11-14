#pragma once

#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

#include <cstring>
#include <string>

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
    if (buf.empty() || buf.back() != '\0') buf.push_back('\0');
  }
};
//@}

//@{
/** @name char[N] formatting */
template <std::size_t N>
struct BufferFormatter<char[N], DataFormat::kTextDataFormat> {
  const char* value;

  explicit BufferFormatter(const char* val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    BufferFormatter<const char*, DataFormat::kTextDataFormat>::WriteN(buf,
                                                                      value, N);
  }
};
//@}

//@{
/** @name std::string */
template <>
struct BufferFormatter<std::string, DataFormat::kTextDataFormat> {
  const std::string& value;

  explicit BufferFormatter(const std::string& val) : value{val} {}
  template <typename Buffer>
  void operator()(Buffer& buf) const {
    buf.reserve(buf.size() + value.size());
    std::copy(value.begin(), value.end(), std::back_inserter(buf));
    if (buf.empty() || buf.back() != '\0') buf.push_back('\0');
  }
};

template <>
struct BufferParser<std::string, DataFormat::kTextDataFormat> {
  std::string& value;

  explicit BufferParser(std::string& val) : value{val} {}

  void operator()(const FieldBuffer& buffer) {
    std::string{buffer.buffer, buffer.length}.swap(value);
  }
};
//@}

// TODO string_view

//@{
/** @name C++ to PostgreSQL mapping for string types */
template <>
struct CppToPg<const char*> : detail::CppToPgPredefined<PredefinedOids::kText> {
};
template <>
struct CppToPg<char*> : detail::CppToPgPredefined<PredefinedOids::kText> {};
template <std::size_t N>
struct CppToPg<char[N]> : detail::CppToPgPredefined<PredefinedOids::kText> {};
template <>
struct CppToPg<std::string> : detail::CppToPgPredefined<PredefinedOids::kText> {
};
//@}

}  // namespace io
}  // namespace postgres
}  // namespace storages
