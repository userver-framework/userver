#pragma once

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/traits.hpp>

namespace storages::postgres::io {

namespace detail {

template <typename T, typename Enable = ::utils::void_t<>>
struct ParserRequiresTypeCategories : std::false_type {};

template <typename T>
struct ParserRequiresTypeCategories<
    T, ::utils::void_t<decltype(std::declval<T&>()(
           std::declval<const FieldBuffer&>(),
           std::declval<const TypeBufferCategory&>()))>> : std::true_type {};

}  // namespace detail

/// @brief Read a value from input buffer
template <DataFormat F, typename T>
void ReadBuffer(const FieldBuffer& buffer, T&& value) {
  using ValueType = std::decay_t<T>;
  static_assert((traits::HasParser<ValueType, F>::value == true),
                "Type doesn't have an appropriate parser");
  using BufferReader = typename traits::IO<ValueType, F>::ParserType;
  static_assert(!detail::ParserRequiresTypeCategories<BufferReader>::value,
                "Type parser requires knowledge about type categories");
  if (traits::kParserBufferCategory<BufferReader> != buffer.category) {
    throw InvalidParserCategory(::compiler::GetTypeName<ValueType>(),
                                traits::kTypeBufferCategory<ValueType>,
                                buffer.category);
  }
  BufferReader{std::forward<T>(value)}(buffer);
}

template <DataFormat F, typename T>
void ReadBuffer(const FieldBuffer& buffer, T&& value,
                const TypeBufferCategory& categories) {
  using ValueType = std::decay_t<T>;
  static_assert((traits::kHasParser<ValueType, F> == true),
                "Type doesn't have an appropriate parser");
  using BufferReader = typename traits::IO<ValueType, F>::ParserType;
  if (traits::kParserBufferCategory<BufferReader> != buffer.category) {
    throw InvalidParserCategory(::compiler::GetTypeName<ValueType>(),
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
void ReadText(const FieldBuffer& buffer, T&& value) {
  using ValueType = std::decay_t<T>;
  static_assert((traits::kHasTextParser<ValueType> == true),
                "Type doesn't have a text parser");
  ReadBuffer<DataFormat::kTextDataFormat>(buffer, std::forward<T>(value));
}

template <typename T>
void ReadBinary(const FieldBuffer& buffer, T&& value) {
  using ValueType = std::decay_t<T>;
  static_assert((traits::kHasBinaryParser<ValueType> == true),
                "Type doesn't have a binary parser");
  ReadBuffer<DataFormat::kBinaryDataFormat>(buffer, std::forward<T>(value));
}

template <typename T>
void ReadBinary(const FieldBuffer& buffer, T&& value,
                const TypeBufferCategory& categories) {
  using ValueType = std::decay_t<T>;
  static_assert((traits::kHasBinaryParser<ValueType> == true),
                "Type doesn't have a binary parser");
  ReadBuffer<DataFormat::kBinaryDataFormat>(buffer, std::forward<T>(value),
                                            categories);
}

/// Helper function to create a buffer reader
template <DataFormat F, typename T>
typename traits::IO<T, F>::FormatterType BufferWriter(const T& value) {
  return typename traits::IO<T, F>::FormatterType(value);
}

template <DataFormat F, typename T, typename Buffer>
void WriteBuffer(const UserTypes& types, Buffer& buffer, const T& value) {
  static_assert((traits::HasFormatter<T, F>::value == true),
                "Type doesn't have an appropriate formatter");
  BufferWriter<F>(value)(types, buffer);
}

template <typename Buffer, typename T>
void WriteBinary(const UserTypes& types, Buffer& buffer, const T& value) {
  static_assert((traits::HasBinaryFormatter<T>::value == true),
                "Type doesn't have a binary formatter");
  BufferWriter<DataFormat::kBinaryDataFormat>(value)(types, buffer);
}

}  // namespace storages::postgres::io
