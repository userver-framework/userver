#pragma once

/// @file userver/storages/postgres/io/bytea.hpp
/// @brief storages::postgres::Bytea I/O support
/// @ingroup userver_postgres_parse_and_format

#include <string>
#include <string_view>
#include <vector>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/**
 * @page pg_bytea uPg bytea support
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
 * ----------
 *
 * @htmlonly <div class="bottom-nav"> @endhtmlonly
 * ⇦ @ref pg_arrays | @ref md_en_userver_mongodb ⇨
 * @htmlonly </div> @endhtmlonly
 */

namespace io::traits {

template <typename T>
struct IsByteaCompatible : std::false_type {};

template <>
struct IsByteaCompatible<std::string> : std::true_type {};
template <>
struct IsByteaCompatible<std::string_view> : std::true_type {};
template <typename... VectorArgs>
struct IsByteaCompatible<std::vector<char, VectorArgs...>> : std::true_type {};
template <typename... VectorArgs>
struct IsByteaCompatible<std::vector<unsigned char, VectorArgs...>>
    : std::true_type {};

template <typename T>
inline constexpr bool kIsByteaCompatible = IsByteaCompatible<T>::value;

template <typename T>
using EnableIfByteaCompatible = std::enable_if_t<IsByteaCompatible<T>{}>;

}  // namespace io::traits

namespace detail {

template <typename ByteContainerRef>
struct ByteaRefWrapper {
  static_assert(std::is_reference<ByteContainerRef>::value,
                "The container must be passed by reference");

  using BytesType = std::decay_t<ByteContainerRef>;
  static_assert(
      io::traits::kIsByteaCompatible<BytesType>,
      "This C++ type cannot be used with PostgreSQL `bytea` data type");

  ByteContainerRef bytes;
};

}  // namespace detail

/// Helper function for writing binary data
template <typename ByteContainer>
detail::ByteaRefWrapper<const ByteContainer&> Bytea(
    const ByteContainer& bytes) {
  return {bytes};
}

/// Helper function for reading binary data
template <typename ByteContainer>
detail::ByteaRefWrapper<ByteContainer&> Bytea(ByteContainer& bytes) {
  return {bytes};
}

/// Wrapper for binary data container
template <typename ByteContainer>
struct ByteaWrapper {
  using BytesType = std::decay_t<ByteContainer>;

  static_assert(
      io::traits::kIsByteaCompatible<BytesType>,
      "This C++ type cannot be used with PostgreSQL `bytea` data type");

  ByteContainer bytes;
};

namespace io {

template <typename ByteContainer>
struct BufferParser<
    postgres::detail::ByteaRefWrapper<ByteContainer>,
    traits::EnableIfByteaCompatible<std::decay_t<ByteContainer>>>
    : detail::BufferParserBase<
          postgres::detail::ByteaRefWrapper<ByteContainer>&&> {
  using BaseType = detail::BufferParserBase<
      postgres::detail::ByteaRefWrapper<ByteContainer>&&>;
  using BaseType::BaseType;
  using ByteaType = postgres::detail::ByteaRefWrapper<ByteContainer>;

  void operator()(const FieldBuffer& buffer) {
    if constexpr (std::is_same<typename ByteaType::BytesType,
                               std::string_view>{}) {
      this->value.bytes = std::string_view{
          reinterpret_cast<const char*>(buffer.buffer), buffer.length};
    } else {
      this->value.bytes.resize(buffer.length);
      std::copy(buffer.buffer, buffer.buffer + buffer.length,
                this->value.bytes.begin());
    }
  }
};

template <typename ByteContainer>
struct BufferParser<
    postgres::ByteaWrapper<ByteContainer>,
    traits::EnableIfByteaCompatible<std::decay_t<ByteContainer>>>
    : detail::BufferParserBase<postgres::ByteaWrapper<ByteContainer>> {
  using BaseType =
      detail::BufferParserBase<postgres::ByteaWrapper<ByteContainer>>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    ReadBuffer(buffer, Bytea(this->value.bytes));
  }
};

template <typename ByteContainer>
struct BufferFormatter<
    postgres::detail::ByteaRefWrapper<ByteContainer>,
    traits::EnableIfByteaCompatible<std::decay_t<ByteContainer>>>
    : detail::BufferFormatterBase<
          postgres::detail::ByteaRefWrapper<ByteContainer>> {
  using BaseType = detail::BufferFormatterBase<
      postgres::detail::ByteaRefWrapper<ByteContainer>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    buf.reserve(buf.size() + this->value.bytes.size());
    buf.insert(buf.end(), this->value.bytes.begin(), this->value.bytes.end());
  }
};

template <typename ByteContainer>
struct BufferFormatter<
    postgres::ByteaWrapper<ByteContainer>,
    traits::EnableIfByteaCompatible<std::decay_t<ByteContainer>>>
    : detail::BufferFormatterBase<postgres::ByteaWrapper<ByteContainer>> {
  using BaseType =
      detail::BufferFormatterBase<postgres::ByteaWrapper<ByteContainer>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WriteBuffer(types, buffer, Bytea(this->value.bytes));
  }
};

template <typename ByteContainer>
struct CppToSystemPg<postgres::detail::ByteaRefWrapper<ByteContainer>>
    : PredefinedOid<PredefinedOids::kBytea> {};
template <typename ByteContainer>
struct CppToSystemPg<postgres::ByteaWrapper<ByteContainer>>
    : PredefinedOid<PredefinedOids::kBytea> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
