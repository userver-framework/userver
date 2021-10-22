#pragma once

/// @file userver/storages/postgres/io/buffer_io.hpp
/// @brief I/O buffer helpers

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace detail {

template <typename T, typename Enable = USERVER_NAMESPACE::utils::void_t<>>
struct ParserRequiresTypeCategories : std::false_type {};

template <typename T>
struct ParserRequiresTypeCategories<
    T, USERVER_NAMESPACE::utils::void_t<decltype(std::declval<T&>()(
           std::declval<const FieldBuffer&>(),
           std::declval<const TypeBufferCategory&>()))>> : std::true_type {};

template <typename T>
inline constexpr bool kParserRequiresTypeCategories =
    ParserRequiresTypeCategories<typename traits::IO<T>::ParserType>::value;

}  // namespace detail

/// @brief Read a value from input buffer
template <typename T>
void ReadBuffer(const FieldBuffer& buffer, T&& value) {
  using ValueType = std::decay_t<T>;
  static_assert(traits::kHasParser<ValueType>, "Type doesn't have a parser");
  using BufferReader = typename traits::IO<ValueType>::ParserType;
  static_assert(!detail::ParserRequiresTypeCategories<BufferReader>::value,
                "Type parser requires knowledge about type categories");
  if (traits::kParserBufferCategory<BufferReader> != buffer.category) {
    throw InvalidParserCategory(compiler::GetTypeName<ValueType>(),
                                traits::kTypeBufferCategory<ValueType>,
                                buffer.category);
  }
  BufferReader{std::forward<T>(value)}(buffer);
}

template <typename T>
void ReadBuffer(const FieldBuffer& buffer, T&& value,
                const TypeBufferCategory& categories) {
  using ValueType = std::decay_t<T>;
  static_assert(traits::kHasParser<ValueType>, "Type doesn't have a parser");
  using BufferReader = typename traits::IO<ValueType>::ParserType;
  if (traits::kParserBufferCategory<BufferReader> != buffer.category) {
    throw InvalidParserCategory(compiler::GetTypeName<ValueType>(),
                                traits::kTypeBufferCategory<ValueType>,
                                buffer.category);
  }
  if constexpr (detail::ParserRequiresTypeCategories<BufferReader>::value) {
    BufferReader{std::forward<T>(value)}(buffer, categories);
  } else {
    BufferReader{std::forward<T>(value)}(buffer);
  }
}

template <typename T>
typename traits::IO<T>::FormatterType BufferWriter(const T& value) {
  return typename traits::IO<T>::FormatterType(value);
}

template <typename T, typename Buffer>
void WriteBuffer(const UserTypes& types, Buffer& buffer, const T& value) {
  static_assert(traits::kHasFormatter<T>, "Type doesn't have a formatter");
  BufferWriter(value)(types, buffer);
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
