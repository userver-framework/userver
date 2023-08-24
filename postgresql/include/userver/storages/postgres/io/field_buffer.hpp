#pragma once

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/nullable_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

inline constexpr FieldBuffer FieldBuffer::GetSubBuffer(
    std::size_t offset, std::size_t size, BufferCategory cat) const {
  const auto* new_buffer_start = buffer + offset;
  if (offset > length) {
    throw InvalidInputBufferSize(
        length, ". Offset requested " + std::to_string(offset));
  }
  size = size == npos ? length - offset : size;
  if (offset + size > length) {
    throw InvalidInputBufferSize(
        size, ". Buffer remaininig size is " + std::to_string(length - offset));
  }
  if (cat == BufferCategory::kKeepCategory) {
    cat = category;
  }
  return {is_null, cat, size, new_buffer_start};
}

template <typename T>
std::size_t FieldBuffer::Read(T&& value, BufferCategory cat, std::size_t len) {
  io::ReadBuffer(GetSubBuffer(0, len, cat), std::forward<T>(value));
  length -= len;
  buffer += len;
  return len;
}

template <typename T>
std::size_t FieldBuffer::Read(T&& value, const TypeBufferCategory& categories,
                              std::size_t len, BufferCategory cat) {
  io::ReadBuffer(GetSubBuffer(0, len, cat), std::forward<T>(value), categories);
  length -= len;
  buffer += len;
  return len;
}

template <typename T>
std::size_t FieldBuffer::ReadRaw(T&& value,
                                 const TypeBufferCategory& categories,
                                 BufferCategory cat) {
  using ValueType = std::decay_t<T>;
  Integer field_length{0};
  auto consumed = Read(field_length, BufferCategory::kPlainBuffer);
  if (field_length == kPgNullBufferSize) {
    // NULL value
    traits::GetSetNull<ValueType>::SetNull(std::forward<T>(value));
    return consumed;
  } else if (field_length < 0) {
    // invalid length value
    throw InvalidInputBufferSize(0, "Negative buffer size value");
  } else if (field_length == 0) {
    traits::GetSetNull<ValueType>::SetDefault(std::forward<T>(value));
    return consumed;
  } else {
    return consumed + Read(value, categories, field_length, cat);
  }
}

template <typename T>
std::size_t ReadRawBinary(FieldBuffer buffer, T& value,
                          const TypeBufferCategory& categories) {
  return buffer.ReadRaw(value, categories);
}

namespace detail {

template <typename T, typename Buffer,
          typename Enable = USERVER_NAMESPACE::utils::void_t<>>
struct FormatterAcceptsReplacementOid : std::false_type {};

template <typename T, typename Buffer>
struct FormatterAcceptsReplacementOid<
    T, Buffer,
    USERVER_NAMESPACE::utils::void_t<decltype(std::declval<T&>()(
        std::declval<const UserTypes&>(), std::declval<Buffer&>(),
        std::declval<Oid>()))>> : std::true_type {};

}  // namespace detail

template <typename T, typename Buffer>
void WriteRawBinary(const UserTypes& types, Buffer& buffer, const T& value,
                    [[maybe_unused]] Oid replace_oid = kInvalidOid) {
  traits::CheckFormatter<T>();
  static constexpr auto size_len = sizeof(Integer);
  if (traits::GetSetNull<T>::IsNull(value)) {
    io::WriteBuffer(types, buffer, kPgNullBufferSize);
  } else {
    using BufferFormatter = typename traits::IO<T>::FormatterType;
    using AcceptsReplacementOid =
        detail::FormatterAcceptsReplacementOid<BufferFormatter, Buffer>;
    auto len_start = buffer.size();
    buffer.resize(buffer.size() + size_len);
    auto size_before = buffer.size();
    if constexpr (AcceptsReplacementOid{}) {
      BufferFormatter{value}(types, buffer, replace_oid);
    } else {
      io::WriteBuffer(types, buffer, value);
    }
    Integer bytes = buffer.size() - size_before;
    BufferWriter(bytes)(buffer.begin() + len_start);
  }
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
