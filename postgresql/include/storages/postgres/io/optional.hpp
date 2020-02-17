#pragma once

/// @file storages/postgres/io/optional.hpp
/// @brief Optional values I/O support

#include <optional>

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/traits.hpp>

#include <boost/optional.hpp>

namespace storages::postgres::io {

namespace detail {

template <template <typename> class Optional, typename T, DataFormat F,
          bool Categories>
struct OptionalValueParser : BufferParserBase<Optional<T>> {
  using BaseType = BufferParserBase<Optional<T>>;
  using ValueParser = typename traits::IO<T, F>::ParserType;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    T val;
    ValueParser{val}(buffer);
    this->value = std::move(val);
  }
};

template <template <typename> class Optional, typename T, DataFormat F>
struct OptionalValueParser<Optional, T, F, true>
    : BufferParserBase<Optional<T>> {
  using BaseType = BufferParserBase<Optional<T>>;
  using ValueParser = typename traits::IO<T, F>::ParserType;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer,
                  const TypeBufferCategory& categories) {
    T val;
    ValueParser{val}(buffer, categories);
    this->value = std::move(val);
  }
};

template <template <typename> class Optional, typename T, DataFormat F>
struct OptionalValueFormatter : BufferFormatterBase<Optional<T>> {
  using BaseType = BufferFormatterBase<Optional<T>>;
  using ValueFormatter = typename traits::IO<T, F>::FormatterType;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    if (this->value) {
      ValueFormatter{*this->value}(types, buffer);
    }
  }
};

}  // namespace detail

/// Parser specialisation for boost::optional
template <typename T, DataFormat F>
struct BufferParser<boost::optional<T>, F,
                    std::enable_if_t<traits::kHasParser<T, F>>>
    : detail::OptionalValueParser<boost::optional, T, F,
                                  detail::kParserRequiresTypeCategories<T>> {
  using BaseType =
      detail::OptionalValueParser<boost::optional, T, F,
                                  detail::kParserRequiresTypeCategories<T>>;
  using BaseType::BaseType;
};

/// Formatter specialisation for boost::optional
template <typename T, DataFormat F>
struct BufferFormatter<boost::optional<T>, F,
                       std::enable_if_t<traits::kHasFormatter<T, F>>>
    : detail::OptionalValueFormatter<boost::optional, T, F> {
  using BaseType = detail::OptionalValueFormatter<boost::optional, T, F>;
  using BaseType::BaseType;
};

/// Parser specialisation for std::optional
template <typename T, DataFormat F>
struct BufferParser<std::optional<T>, F,
                    std::enable_if_t<traits::kHasParser<T, F>>>
    : detail::OptionalValueParser<std::optional, T, F,
                                  detail::kParserRequiresTypeCategories<T>> {
  using BaseType =
      detail::OptionalValueParser<std::optional, T, F,
                                  detail::kParserRequiresTypeCategories<T>>;
  using BaseType::BaseType;
};

/// Formatter specialisation for std::optional
template <typename T, DataFormat F>
struct BufferFormatter<std::optional<T>, F,
                       std::enable_if_t<traits::kHasFormatter<T, F>>>
    : detail::OptionalValueFormatter<std::optional, T, F> {
  using BaseType = detail::OptionalValueFormatter<std::optional, T, F>;
  using BaseType::BaseType;
};

/// Pg mapping specialisation for boost::optional
template <typename T>
struct CppToPg<boost::optional<T>, std::enable_if_t<traits::kIsMappedToPg<T>>>
    : CppToPg<T> {};

/// Pg mapping specialisation for std::optional
template <typename T>
struct CppToPg<std::optional<T>, std::enable_if_t<traits::kIsMappedToPg<T>>>
    : CppToPg<T> {};

namespace traits {

/// Nullability traits for boost::optional
template <typename T>
struct IsNullable<boost::optional<T>> : std::true_type {};

template <typename T>
struct GetSetNull<boost::optional<T>> {
  using ValueType = boost::optional<T>;
  inline static bool IsNull(const ValueType& v) { return !v; }
  inline static void SetNull(ValueType& v) { v = boost::none; }
  inline static void SetDefault(ValueType& v) { v.emplace(); }
};

template <typename T>
struct IsMappedToPg<boost::optional<T>> : IsMappedToPg<T> {};
template <typename T>
struct IsSpecialMapping<boost::optional<T>> : IsMappedToPg<T> {};

template <typename T>
struct ParserBufferCategory<
    BufferParser<boost::optional<T>, DataFormat::kBinaryDataFormat>>
    : ParserBufferCategory<
          typename traits::IO<T, DataFormat::kBinaryDataFormat>::ParserType> {};

/// Nullability traits for std::optional
template <typename T>
struct IsNullable<std::optional<T>> : std::true_type {};

template <typename T>
struct GetSetNull<std::optional<T>> {
  using ValueType = std::optional<T>;
  inline static bool IsNull(const ValueType& v) { return !v; }
  inline static void SetNull(ValueType& v) { v = std::nullopt; }
  inline static void SetDefault(ValueType& v) { v.emplace(); }
};

template <typename T>
struct IsMappedToPg<std::optional<T>> : IsMappedToPg<T> {};
template <typename T>
struct IsSpecialMapping<std::optional<T>> : IsMappedToPg<T> {};

template <typename T>
struct ParserBufferCategory<
    BufferParser<std::optional<T>, DataFormat::kBinaryDataFormat>>
    : ParserBufferCategory<
          typename traits::IO<T, DataFormat::kBinaryDataFormat>::ParserType> {};

}  // namespace traits

}  // namespace storages::postgres::io
