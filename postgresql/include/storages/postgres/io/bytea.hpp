#pragma once

#include <string>
#include <vector>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/type_mapping.hpp>

#include <utils/string_view.hpp>

namespace storages::postgres {

/**
 * @page pg_bytea µPg bytea support
 *
 * The driver allows reading and writing raw binary data from/to PostgreSQL
 * `bytea` type.
 *
 * Reading and writing to PostgreSQL is implemented for `std::string`,
 * `std::string_view` and `std::vector` of `char` or `unsigned char`.
 *
 * @warning When reading to `std::string_view` the value MUST NOT be used after
 * the PostgreSQL result set is destroyed.
 *
 * @code
 * namespace pg = storages::postgres;
 * using namespace std::string_literals;
 * std::string s = "\0\xff\x0afoobar"s;
 * trx.Execute("select $1", pg::Bytea(tp));
 * @endcode
 *
 */

namespace io::traits {

template <typename T>
struct IsByteaCompatible : std::false_type {};

template <>
struct IsByteaCompatible<std::string> : std::true_type {};
template <>
struct IsByteaCompatible<::utils::string_view> : std::true_type {};
template <typename... VectorArgs>
struct IsByteaCompatible<std::vector<char, VectorArgs...>> : std::true_type {};
template <typename... VectorArgs>
struct IsByteaCompatible<std::vector<unsigned char, VectorArgs...>>
    : std::true_type {};

template <typename T>
constexpr bool kIsByteaCompatible = IsByteaCompatible<T>::value;

template <typename T>
using EnableIfByteaCompatible = std::enable_if_t<IsByteaCompatible<T>{}>;

}  // namespace io::traits

namespace detail {

template <typename ByteContainerRef>
struct Bytea {
  static_assert(std::is_reference<ByteContainerRef>::value,
                "The container must be passed by reference");
  using BytesType = std::decay_t<ByteContainerRef>;
  ByteContainerRef bytes;
};

}  // namespace detail

/// Helper function for writing binary data
template <typename ByteContainer>
detail::Bytea<const ByteContainer&> Bytea(const ByteContainer& bytes) {
  static_assert(
      io::traits::kIsByteaCompatible<ByteContainer>,
      "This C++ type cannot be used with PostgreSQL `bytea` data type");
  return {bytes};
}

/// Helper function for reading binary data
template <typename ByteContainer>
detail::Bytea<ByteContainer&> Bytea(ByteContainer& bytes) {
  static_assert(
      io::traits::kIsByteaCompatible<ByteContainer>,
      "This C++ type cannot be used with PostgreSQL `bytea` data type");
  return {bytes};
}

namespace io {

template <typename ByteContainer>
struct BufferFormatter<
    postgres::detail::Bytea<ByteContainer>, DataFormat::kBinaryDataFormat,
    traits::EnableIfByteaCompatible<std::decay_t<ByteContainer>>>
    : detail::BufferFormatterBase<postgres::detail::Bytea<ByteContainer>> {
  using BaseType =
      detail::BufferFormatterBase<postgres::detail::Bytea<ByteContainer>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    buf.reserve(buf.size() + this->value.bytes.size());
    buf.insert(buf.end(), this->value.bytes.begin(), this->value.bytes.end());
  }
};

template <typename ByteContainer>
struct BufferParser<
    postgres::detail::Bytea<ByteContainer>, DataFormat::kBinaryDataFormat,
    traits::EnableIfByteaCompatible<std::decay_t<ByteContainer>>>
    : detail::BufferParserBase<postgres::detail::Bytea<ByteContainer>&&> {
  using BaseType =
      detail::BufferParserBase<postgres::detail::Bytea<ByteContainer>&&>;
  using BaseType::BaseType;
  using ByteaType = postgres::detail::Bytea<ByteContainer>;

  void operator()(const FieldBuffer& buffer) {
    if constexpr (std::is_same<typename ByteaType::BytesType,
                               ::utils::string_view>{}) {
      this->value.bytes = ::utils::string_view{
          reinterpret_cast<const char*>(buffer.buffer), buffer.length};
    } else {
      this->value.bytes.resize(buffer.length);
      std::copy(buffer.buffer, buffer.buffer + buffer.length,
                this->value.bytes.begin());
    }
  }
};

template <typename ByteContainer>
struct CppToSystemPg<postgres::detail::Bytea<ByteContainer>>
    : PredefinedOid<PredefinedOids::kBytea> {};

}  // namespace io
}  // namespace storages::postgres
