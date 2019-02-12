#pragma once

#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <storages/postgres/detail/is_decl_complete.hpp>
#include <storages/postgres/io/pg_types.hpp>
#include <utils/void_t.hpp>

namespace storages::postgres {

class UserTypes;

namespace io {

/// @page pg_io µPg: Parsers and Formatters
///
/// @todo Decribe the system of parser, formatters and their customization
///

enum DataFormat { kTextDataFormat = 0, kBinaryDataFormat = 1 };

/// Category of buffer contents.
///
/// Applied to binary parsers and deduced from field's data type.
enum class BufferCategory {
  kNoParser,  //!< kNoParser the data type doesn't have a parser defined
  kVoid,  //!< kVoid there won't be a buffer for this field, but the category is
          //!< required for correctly handling void results
  kPlainBuffer,      //!< kPlainBuffer the buffer is a single plain value
  kArrayBuffer,      //!< kArrayBuffer the buffer contains an array of values
  kCompositeBuffer,  //!< kCompositeBuffer the buffer contains a user-defined
                     //!< composite type
  kRangeBuffer       //!< kRangeBuffer the buffer contains a range type
};

const std::string& ToString(BufferCategory);

template <BufferCategory Category>
using BufferCategoryConstant = std::integral_constant<BufferCategory, Category>;

using TypeBufferCategory = std::unordered_map<Oid, BufferCategory>;

BufferCategory GetTypeBufferCategory(const TypeBufferCategory&, Oid);

struct BufferCategoryHash {
  using IntegerType = std::underlying_type_t<BufferCategory>;
  std::size_t operator()(BufferCategory val) const {
    return std::hash<IntegerType>{}(static_cast<IntegerType>(val));
  }
};

/// Fields that are null are denoted by specifying their length == -1
constexpr const Integer kPgNullBufferSize = -1;

struct FieldBuffer {
  static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

  bool is_null = false;
  DataFormat format = DataFormat::kTextDataFormat;
  BufferCategory category = BufferCategory::kPlainBuffer;
  std::size_t length = 0;
  const std::uint8_t* buffer = nullptr;

  std::string ToString() const {
    return {reinterpret_cast<const char*>(buffer), length};
  }
  constexpr FieldBuffer GetSubBuffer(
      std::size_t offset, std::size_t size = npos,
      BufferCategory cat = BufferCategory::kNoParser) const;
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
template <typename T>
constexpr bool kHasAnyParser = kHasTextParser<T> || kHasBinaryParser<T>;

template <typename T, DataFormat F>
constexpr bool kHasFormatter = HasFormatter<T, F>::value;
template <typename T>
constexpr bool kHasTextFormatter = HasTextFormatter<T>::value;
template <typename T>
constexpr bool kHasBinaryFormatter = HasBinaryFormatter<T>::value;
template <typename T>
constexpr bool kHasAnyFormatter =
    kHasTextFormatter<T> || kHasBinaryFormatter<T>;
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

template <typename T>
using BestParserType = typename BestParser<T>::type;

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

template <typename T>
using BestFormatterType = typename BestFormatter<T>::type;

/// Buffer category for parser
template <typename T>
struct ParserBufferCategory
    : BufferCategoryConstant<BufferCategory::kPlainBuffer> {};
template <typename T>
using ParserBufferCategoryType = typename ParserBufferCategory<T>::type;
template <typename T>
constexpr BufferCategory kParserBufferCategory = ParserBufferCategory<T>::value;

//@{
/** @name Buffer category for a type */
namespace detail {
template <typename T>
constexpr auto DetectBufferCategory() {
  if constexpr (kHasAnyParser<T>) {
    return ParserBufferCategoryType<BestParserType<T>>{};
  } else {
    return BufferCategoryConstant<BufferCategory::kNoParser>{};
  }
};
}  // namespace detail
template <typename T>
struct TypeBufferCategory : decltype(detail::DetectBufferCategory<T>()) {};
template <typename T>
constexpr BufferCategory kTypeBufferCategory = TypeBufferCategory<T>::value;
//@}

namespace detail {

template <typename T, DataFormat F>
struct CustomParserDefined : utils::IsDeclComplete<BufferParser<T, F>> {};
template <typename T, DataFormat F>
constexpr bool kCustomParserDefined = CustomParserDefined<T, F>::value;

template <typename T>
using CustomTextParserDefined =
    CustomParserDefined<T, DataFormat::kTextDataFormat>;
template <typename T>
using CustomBinaryParserDefined =
    CustomParserDefined<T, DataFormat::kBinaryDataFormat>;

template <typename T>
constexpr bool kCustomTextParserDefined = CustomTextParserDefined<T>::value;
template <typename T>
constexpr bool kCustomBinaryParserDefined = CustomBinaryParserDefined<T>::value;

template <typename T, DataFormat F>
struct CustomFormatterDefined : utils::IsDeclComplete<BufferFormatter<T, F>> {};
template <typename T, DataFormat F>
constexpr bool kCustomFormatterDefined = CustomFormatterDefined<T, F>::value;

template <typename T>
using CustomTextFormatterDefined =
    CustomFormatterDefined<T, DataFormat::kTextDataFormat>;
template <typename T>
using CustomBinaryFormatterDefined =
    CustomFormatterDefined<T, DataFormat::kBinaryDataFormat>;

template <typename T>
constexpr bool kCustomTextFormatterDefined =
    CustomTextFormatterDefined<T>::value;
template <typename T>
constexpr bool kCustomBinaryFormatterDefined =
    CustomBinaryFormatterDefined<T>::value;

}  // namespace detail

}  // namespace traits

}  // namespace io
}  // namespace storages::postgres
