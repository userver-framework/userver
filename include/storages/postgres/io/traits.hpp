#pragma once

#include <type_traits>

#include <storages/postgres/detail/is_decl_complete.hpp>
#include <storages/postgres/io/pg_types.hpp>
#include <utils/void_t.hpp>

namespace storages {
namespace postgres {
namespace io {

enum DataFormat { kTextDataFormat = 0, kBinaryDataFormat = 1 };

struct FieldBuffer {
  bool is_null;
  DataFormat format;
  std::size_t length;
  const char* buffer;
};

/// @brief Primary template for Postgre buffer parser.
/// Specialisations should provide call operators that parse FieldBuffer.
template <typename T, DataFormat, typename Enable = ::utils::void_t<>>
struct BufferParser;

/// @brief Primary template for Postgre buffer formatter
/// Specialisations should provide call operators that write to a buffer.
template <typename T, DataFormat, typename Enable = ::utils::void_t<>>
struct BufferFormatter;

namespace traits {

/// Customisation point for parsers
template <typename T, DataFormat F, typename Enable = ::utils::void_t<>>
struct Input {
  using type = BufferParser<T, F>;
};

/// Customisation point for formatters
template <typename T, DataFormat F, typename Enable = ::utils::void_t<>>
struct Output {
  using type = BufferFormatter<T, F>;
};

/// @brief A default deducer of parsers/formatters for a type/data format.
/// Can be specialised for a type/format pair providing custom
/// parsers/formatters.
template <typename T, DataFormat F>
struct IO {
  using ParserType = typename Input<T, F>::type;
  using FormatterType = typename Output<T, F>::type;
};

/// @brief Metafunction to detect if a type has a parser of particular
/// data format.
template <typename T, DataFormat Format>
struct HasParser : utils::IsDeclComplete<typename IO<T, Format>::ParserType> {};

/// @brief Metafunction to detect if a type has a formatter to particular
/// data format.
template <typename T, DataFormat Format>
struct HasFormatter
    : utils::IsDeclComplete<typename IO<T, Format>::FormatterType> {};

//@{
/** @name Shortcut metafunctions, mostly for tests. */
template <typename T>
using HasTextParser = HasParser<T, DataFormat::kTextDataFormat>;
template <typename T>
using HasBinaryParser = HasParser<T, DataFormat::kBinaryDataFormat>;

template <typename T>
using HasTextFormatter = HasFormatter<T, DataFormat::kTextDataFormat>;
template <typename T>
using HasBinaryFormatter = HasFormatter<T, DataFormat::kBinaryDataFormat>;
//@}

//@{
/** @name Shortcut metafunction result values */
template <typename T, DataFormat F>
constexpr bool kHasParser = HasParser<T, F>::value;
template <typename T>
constexpr bool kHasTextParser = HasTextParser<T>::value;
template <typename T>
constexpr bool kHasBinaryParser = HasBinaryParser<T>::value;

template <typename T, DataFormat F>
constexpr bool kHasFormatter = HasFormatter<T, F>::value;
template <typename T>
constexpr bool kHasTextFormatter = HasTextFormatter<T>::value;
template <typename T>
constexpr bool kHasBinaryFormatter = HasBinaryFormatter<T>::value;
//@}

/// Parser selector
template <typename T>
struct BestParser {
 private:
  static constexpr bool has_binary_parser =
      HasParser<T, kBinaryDataFormat>::value;

 public:
  static constexpr DataFormat value =
      has_binary_parser ? kBinaryDataFormat : kTextDataFormat;
  static_assert((HasParser<T, value>::value), "No parser defined for type");
  using type = typename IO<T, value>::ParserType;
};

/// Formatter selector
template <typename T>
struct BestFormatter {
 private:
  static constexpr bool has_binary_formatter =
      HasFormatter<T, kBinaryDataFormat>::value;

 public:
  static constexpr DataFormat value =
      has_binary_formatter ? kBinaryDataFormat : kTextDataFormat;
  static_assert((HasFormatter<T, value>::value),
                "No formatter defined for type");
  using type = typename IO<T, value>::FormatterType;
};

namespace detail {

template <typename T, DataFormat F>
struct CustomParserDefined : utils::IsDeclComplete<BufferParser<T, F>> {};

template <typename T, DataFormat F>
struct CustomFormatterDefined : utils::IsDeclComplete<BufferFormatter<T, F>> {};

}  // namespace detail

}  // namespace traits

/// Helper function to create a buffer parser
template <DataFormat F, typename T>
typename traits::IO<T, F>::ParserType BufferReader(T& value) {
  return typename traits::IO<T, F>::ParserType(value);
}

/// @brief Read a value from input buffer
template <DataFormat F, typename T>
void ReadBuffer(const FieldBuffer& buffer, T& value) {
  static_assert((traits::HasParser<T, F>::value == true),
                "Type doesn't have an appropriate parser");
  BufferReader<F>(value)(buffer);
}

/// Helper function to create a buffer reader
template <DataFormat F, typename T>
typename traits::IO<T, F>::FormatterType BufferWriter(const T& value) {
  return typename traits::IO<T, F>::FormatterType(value);
}

template <DataFormat F, typename T, typename Buffer>
void WriteBuffer(Buffer& buffer, const T& value) {
  static_assert((traits::HasFormatter<T, F>::value == true),
                "Type doesn't have an appropriate formatter");
  BufferWriter<F>(value)(buffer);
}

}  // namespace io
}  // namespace postgres
}  // namespace storages
