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

#ifndef NDEBUG

class ReadersRegistrator final {
 public:
  ReadersRegistrator(std::type_index type, std::type_index parser_type,
                     const char* base_file);
  void RequireInstance() const;
};

// Make instances with different __BASE_FILE__ differ for linker
// NOLINTNEXTLINE(cert-dcl59-cpp,google-build-namespaces)
namespace {

template <class Type, class Reader>
struct CheckForBufferReaderODR final {
  static inline ReadersRegistrator content{typeid(Type), typeid(Reader),
                                           __BASE_FILE__};
};

}  // namespace

class WritersRegistrator final {
 public:
  WritersRegistrator(std::type_index type, std::type_index formatter_type,
                     const char* base_file);
  void RequireInstance() const;
};

// Make instances with different __BASE_FILE__ differ for linker
// NOLINTNEXTLINE(cert-dcl59-cpp,google-build-namespaces)
namespace {

template <class Type, class Writer>
struct CheckForBufferWriterODR final {
  static inline WritersRegistrator content{typeid(Type), typeid(Writer),
                                           __BASE_FILE__};
};

}  // namespace
#endif

}  // namespace detail

/// @brief Read a value from input buffer
template <typename T>
void ReadBuffer(const FieldBuffer& buffer, T&& value) {
  using ValueType = std::decay_t<T>;
  traits::CheckParser<ValueType>();
  using BufferReader = typename traits::IO<ValueType>::ParserType;
  static_assert(!detail::ParserRequiresTypeCategories<BufferReader>::value,
                "Type parser requires knowledge about type categories");
  if (traits::kParserBufferCategory<BufferReader> != buffer.category) {
    throw InvalidParserCategory(compiler::GetTypeName<ValueType>(),
                                traits::kTypeBufferCategory<ValueType>,
                                buffer.category);
  }

#ifndef NDEBUG
  detail::CheckForBufferReaderODR<T, BufferReader>::content.RequireInstance();
#endif
  BufferReader{std::forward<T>(value)}(buffer);
}

template <typename T>
void ReadBuffer(const FieldBuffer& buffer, T&& value,
                const TypeBufferCategory& categories) {
  using ValueType = std::decay_t<T>;
  traits::CheckParser<ValueType>();
  using BufferReader = typename traits::IO<ValueType>::ParserType;
  if (traits::kParserBufferCategory<BufferReader> != buffer.category) {
    throw InvalidParserCategory(compiler::GetTypeName<ValueType>(),
                                traits::kTypeBufferCategory<ValueType>,
                                buffer.category);
  }

#ifndef NDEBUG
  detail::CheckForBufferReaderODR<T, BufferReader>::content.RequireInstance();
#endif

  if constexpr (detail::ParserRequiresTypeCategories<BufferReader>::value) {
    BufferReader{std::forward<T>(value)}(buffer, categories);
  } else {
    BufferReader{std::forward<T>(value)}(buffer);
  }
}

template <typename T>
typename traits::IO<T>::FormatterType BufferWriter(const T& value) {
  using Formatter = typename traits::IO<T>::FormatterType;
#ifndef NDEBUG
  detail::CheckForBufferWriterODR<T, Formatter>::content.RequireInstance();
#endif
  return Formatter(value);
}

template <typename T, typename Buffer>
void WriteBuffer(const UserTypes& types, Buffer& buffer, const T& value) {
  traits::CheckFormatter<T>();
  BufferWriter(value)(types, buffer);
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
