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

template <typename T, typename Buffer>
void WriteRawBinary(const UserTypes& types, Buffer& buffer, const T& value) {
  static_assert(
      (traits::HasFormatter<T, DataFormat::kBinaryDataFormat>::value == true),
      "Type doesn't have a binary formatter");
  static constexpr auto size_len = sizeof(Integer);
  if (traits::GetSetNull<T>::IsNull(value)) {
    WriteBinary(types, buffer, kPgNullBufferSize);
  } else {
    auto len_start = buffer.size();
    buffer.resize(buffer.size() + size_len);
    auto size_before = buffer.size();
    WriteBuffer<DataFormat::kBinaryDataFormat>(types, buffer, value);
    Integer bytes = buffer.size() - size_before;
    BufferWriter<DataFormat::kBinaryDataFormat>(bytes)(buffer.begin() +
                                                       len_start);
  }
}

}  // namespace storages::postgres::io
