#pragma once

/// @file userver/storages/postgres/io/macaddr.hpp
/// @brief utils::Macaddr and utils::Macaddr8 I/O support
/// @ingroup userver_postgres_parse_and_format

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/utils/macaddr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

using Macaddr = USERVER_NAMESPACE::utils::Macaddr;
using Macaddr8 = USERVER_NAMESPACE::utils::Macaddr8;

namespace io {
namespace detail {

template <typename T>
struct MacaddrFormatterBase : BufferFormatterBase<T> {
  using BaseType = BufferFormatterBase<T>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) {
    for (const auto val : this->value.GetOctets()) {
      io::WriteBuffer(types, buffer, static_cast<char>(val));
    }
  }
};

template <typename T>
struct MacaddrBufferParser : BufferParserBase<T> {
  using BaseType = BufferParserBase<T>;
  using BaseType::BaseType;
  void operator()(FieldBuffer buffer) {
    typename T::OctetsType octets;
    const uint8_t* byte_cptr = buffer.buffer;
    for (auto& val : octets) {
      val = *byte_cptr;
      ++byte_cptr;
    }
    this->value = T(octets);
  }
};

}  // namespace detail

///@brief Binary formatter for utils::macaddr:Macaddr
template <>
struct BufferFormatter<Macaddr> : detail::MacaddrFormatterBase<Macaddr> {
  using BaseType = detail::MacaddrFormatterBase<Macaddr>;

  using BaseType::BaseType;
};

///@brief Binary formatter for utils::macaddr:Macaddr8
template <>
struct BufferFormatter<Macaddr8> : detail::MacaddrFormatterBase<Macaddr8> {
  using BaseType = detail::MacaddrFormatterBase<Macaddr8>;

  using BaseType::BaseType;
};

/// @brief Binary parser for utils::macaddr::Macaddr
template <>
struct BufferParser<Macaddr> : detail::MacaddrBufferParser<Macaddr> {
  using BaseType = detail::MacaddrBufferParser<Macaddr>;

  using BaseType::BaseType;
};

/// @brief Binary parser for utils::macaddr::Macaddr8
template <>
struct BufferParser<Macaddr8> : detail::MacaddrBufferParser<Macaddr8> {
  using BaseType = detail::MacaddrBufferParser<Macaddr8>;

  using BaseType::BaseType;
};

template <>
struct CppToSystemPg<Macaddr> : PredefinedOid<PredefinedOids::kMacaddr> {};

template <>
struct CppToSystemPg<Macaddr8> : PredefinedOid<PredefinedOids::kMacaddr8> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
