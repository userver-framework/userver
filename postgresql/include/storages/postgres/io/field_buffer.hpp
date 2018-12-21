#pragma once

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/traits.hpp>

namespace storages::postgres::io {

constexpr FieldBuffer FieldBuffer::GetSubBuffer(std::size_t offset,
                                                std::size_t len) const {
  auto new_buffer_start = buffer + offset;
  if (offset > length) {
    throw InvalidInputBufferSize(
        length, ". Offset requested " + std::to_string(offset));
  }
  len = len == npos ? length - offset : len;
  if (offset + len > length) {
    throw InvalidInputBufferSize(
        len, ". Buffer remaininig size is " + std::to_string(length - offset));
  }
  return {is_null, format, len, new_buffer_start};
}

template <typename T>
std::size_t ReadRawBinary(const FieldBuffer& buffer, T& value) {
  static constexpr auto size_len = sizeof(Integer);
  Integer length{0};
  ReadBinary(buffer.GetSubBuffer(0, size_len), length);
  if (length == kPgNullBufferSize) {
    // NULL value
    traits::GetSetNull<T>::SetNull(value);
    return size_len;
  } else if (length < 0) {
    // invalid length value
    throw InvalidInputBufferSize(0, "Negative buffer size value");
  } else if (length == 0) {
    traits::GetSetNull<T>::SetDefault(value);
    return size_len;
  } else {
    ReadBinary(buffer.GetSubBuffer(size_len, length), value);
    return length + size_len;
  }
}

namespace detail {

template <typename T, typename Buffer, typename Enable = ::utils::void_t<>>
struct FormatterAcceptsReplacementOid : std::false_type {};

template <typename T, typename Buffer>
struct FormatterAcceptsReplacementOid<
    T, Buffer,
    ::utils::void_t<decltype(std::declval<T&>()(
        std::declval<const UserTypes&>(), std::declval<Buffer&>(),
        std::declval<Oid>()))>> : std::true_type {};

}  // namespace detail

template <typename T, typename Buffer>
void WriteRawBinary(const UserTypes& types, Buffer& buffer, const T& value,
                    Oid replace_oid = kInvalidOid) {
  static_assert(
      (traits::HasFormatter<T, DataFormat::kBinaryDataFormat>::value == true),
      "Type doesn't have a binary formatter");
  static constexpr auto size_len = sizeof(Integer);
  if (traits::GetSetNull<T>::IsNull(value)) {
    WriteBinary(types, buffer, kPgNullBufferSize);
  } else {
    using BufferFormatter =
        typename traits::IO<T, DataFormat::kBinaryDataFormat>::FormatterType;
    using AcceptsReplacementOid =
        detail::FormatterAcceptsReplacementOid<BufferFormatter, Buffer>;
    auto len_start = buffer.size();
    buffer.resize(buffer.size() + size_len);
    auto size_before = buffer.size();
    if constexpr (AcceptsReplacementOid{}) {
      BufferFormatter{value}(types, buffer, replace_oid);
    } else {
      WriteBuffer<DataFormat::kBinaryDataFormat>(types, buffer, value);
    }
    Integer bytes = buffer.size() - size_before;
    BufferWriter<DataFormat::kBinaryDataFormat>(bytes)(buffer.begin() +
                                                       len_start);
  }
}

}  // namespace storages::postgres::io
