#pragma once

/// @file userver/storages/postgres/io/network.hpp
/// @brief utils::ip::NetworkV4 I/O support
/// @ingroup userver_postgres_parse_and_format

#include <limits>

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/string_types.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/utils/ip.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

// Corresponds to NetworkV4
using NetworkV4 = USERVER_NAMESPACE::utils::ip::NetworkV4;

// Corresponds to AddressV4
using AddressV4 = USERVER_NAMESPACE::utils::ip::AddressV4;

using NetworkV6 = USERVER_NAMESPACE::utils::ip::NetworkV6;

using AddressV6 = USERVER_NAMESPACE::utils::ip::AddressV6;

using InetNetwork = USERVER_NAMESPACE::utils::ip::InetNetwork;

namespace io {

namespace detail {
// Corresponds to postgresql address family
inline constexpr char kPgsqlAfInet = AF_INET + 0;
inline constexpr char kPgsqlAfInet6 = AF_INET + 1;

// Corresponds to postgresql is_cidr flag
inline constexpr char kIsCidr = 1;
inline constexpr char kIsInet = 0;

template <typename T>
inline constexpr bool kIsNetworkType =
    std::is_same_v<T, USERVER_NAMESPACE::utils::ip::NetworkV4> ||
    std::is_same_v<T, USERVER_NAMESPACE::utils::ip::NetworkV6>;

template <typename T>
inline constexpr bool kIsAddressType =
    std::is_same_v<T, USERVER_NAMESPACE::utils::ip::AddressV4> ||
    std::is_same_v<T, USERVER_NAMESPACE::utils::ip::AddressV6>;

template <typename T>
struct IpBufferFormatterBase : BufferFormatterBase<T> {
 protected:
  using BaseType = BufferFormatterBase<T>;
  using BaseType::BaseType;
  template <typename Address>
  struct IpFormatterInfo {
    Address address;
    char address_family = '\0';
    char prefix_length = '\0';
    char is_cidr = '\0';
  };

  template <typename Buffer, typename Address>
  void Format(IpFormatterInfo<Address> info, const UserTypes& types,
              Buffer& buffer) {
    buffer.reserve(buffer.size() + info.address.size() + 4);
    io::WriteBuffer(types, buffer, info.address_family);
    io::WriteBuffer(types, buffer, info.prefix_length);
    io::WriteBuffer(types, buffer, info.is_cidr);
    io::WriteBuffer(types, buffer, static_cast<char>(info.address.size()));
    for (const auto val : info.address) {
      io::WriteBuffer(types, buffer, static_cast<char>(val));
    }
  }
};

template <typename T, typename = std::enable_if_t<kIsAddressType<T>>>
struct AddressNetworkBuffer : IpBufferFormatterBase<T> {
  using BaseType = IpBufferFormatterBase<T>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) {
    using Address = typename T::BytesType;
    constexpr bool is_address_v4 =
        std::is_same_v<T, USERVER_NAMESPACE::utils::ip::AddressV4>;
    typename BaseType::template IpFormatterInfo<Address> info{
        /* .address = */ this->value.GetBytes(),
        /* .address_family = */ is_address_v4 ? kPgsqlAfInet : kPgsqlAfInet6,
        /* .prefix_length = */
        static_cast<char>(is_address_v4 ? NetworkV4::kMaximumPrefixLength
                                        : NetworkV6::kMaximumPrefixLength),
        /* .is_cidr = */ kIsCidr};
    BaseType::Format(info, types, buffer);
  }
};

template <typename T, typename = std::enable_if_t<kIsNetworkType<T>>>
struct NetworkBufferFormatter : IpBufferFormatterBase<T> {
  using BaseType = IpBufferFormatterBase<T>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) {
    using Address = typename T::AddressType::BytesType;
    const auto canonical_network =
        USERVER_NAMESPACE::utils::ip::TransformToCidrFormat(this->value);
    if (canonical_network != this->value) {
      throw IpAddressInvalidFormat(
          "Network expected CIDR format. Use utils::ip::TransformToCidrFormat "
          "method to conversation.");
    }
    typename BaseType::template IpFormatterInfo<Address> info{
        /* .address = */ canonical_network.GetAddress().GetBytes(),
        /* .address_family = */
        std::is_same_v<T, USERVER_NAMESPACE::utils::ip::NetworkV4>
            ? kPgsqlAfInet
            : kPgsqlAfInet6,
        /* .prefix_length = */
        static_cast<char>(canonical_network.GetPrefixLength()),
        /* .is_cidr = */ kIsCidr};
    BaseType::Format(info, types, buffer);
  }
};

template <typename T>
struct IpBufferParserBase : BufferParserBase<T> {
 protected:
  using BaseType = BufferParserBase<T>;
  using BaseType::BaseType;

  template <typename Bytes>
  struct IpParserInfo {
    Bytes bytes{};
    unsigned char family = '\0';
    unsigned char prefix_length = '\0';
    unsigned char is_cidr = '\0';
    unsigned char bytes_number = '\0';
  };

  template <typename Bytes>
  IpParserInfo<Bytes> Parse(FieldBuffer buffer) {
    IpParserInfo<Bytes> result;
    const uint8_t* byte_cptr = buffer.buffer;
    result.family = *byte_cptr;
    ++byte_cptr;
    result.prefix_length = *byte_cptr;
    ++byte_cptr;
    result.is_cidr = *byte_cptr;
    ++byte_cptr;
    result.bytes_number = *byte_cptr;
    ++byte_cptr;
    this->ParseIpBytes(byte_cptr, result.bytes, result.bytes_number,
                       result.family);
    return result;
  }

 private:
  template <size_t N>
  void ParseIpBytes(const uint8_t* byte_cptr,
                    std::array<unsigned char, N>& bytes,
                    unsigned char bytes_number, unsigned char) {
    if (bytes_number != bytes.size()) {
      throw storages::postgres::IpAddressInvalidFormat(
          fmt::format("Expected address size is {}, actual is {}", bytes_number,
                      bytes.size()));
    }
    std::memcpy(bytes.data(), byte_cptr, bytes.size());
  }

  void ParseIpBytes(const uint8_t* byte_cptr, std::vector<unsigned char>& bytes,
                    unsigned char bytes_number, unsigned char address_family) {
    if (!(bytes_number == 16 && address_family == kPgsqlAfInet6) &&
        !(bytes_number == 4 && address_family == kPgsqlAfInet)) {
      throw storages::postgres::IpAddressInvalidFormat("Invalid INET format");
    }
    bytes.resize(bytes_number);
    std::memcpy(bytes.data(), byte_cptr, bytes_number);
  }
};

template <typename T, typename = std::enable_if_t<kIsNetworkType<T>>>
struct NetworkBufferParser : IpBufferParserBase<T> {
  using BaseType = IpBufferParserBase<T>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    using Address = typename T::AddressType;
    using Bytes = typename Address::BytesType;
    const auto info = BaseType::template Parse<Bytes>(buffer);
    constexpr auto expected_family =
        std::is_same_v<T, NetworkV4> ? kPgsqlAfInet : kPgsqlAfInet6;
    if (info.family != expected_family) {
      throw storages::postgres::IpAddressInvalidFormat(
          "Actual address family doesn't supported for type");
    }
    if (info.is_cidr != kIsCidr) {
      throw storages::postgres::IpAddressInvalidFormat(
          "Network isn't in CIDR format");
    }
    this->value = T(Address(info.bytes), info.prefix_length);
  }
};

template <typename T, typename = std::enable_if_t<kIsAddressType<T>>>
struct AddressBufferParser : detail::IpBufferParserBase<T> {
  using BaseType = detail::IpBufferParserBase<T>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    using Bytes = typename T::BytesType;
    const auto info = BaseType::template Parse<Bytes>(buffer);
    constexpr auto expected_family =
        std::is_same_v<T, AddressV4> ? kPgsqlAfInet : kPgsqlAfInet6;
    if (info.family != expected_family) {
      throw storages::postgres::IpAddressInvalidFormat(
          "Actual address family doesn't supported for type");
    }
    constexpr unsigned char expected_prefix_length =
        std::is_same_v<T, AddressV4> ? NetworkV4::kMaximumPrefixLength
                                     : NetworkV6::kMaximumPrefixLength;
    if (info.prefix_length != expected_prefix_length) {
      throw storages::postgres::IpAddressInvalidFormat(
          fmt::format("Expected prefix length is {}, actual prefix is {}",
                      static_cast<int>(expected_prefix_length),
                      static_cast<int>(info.prefix_length)));
    }
    if (info.is_cidr != kIsCidr) {
      throw storages::postgres::IpAddressInvalidFormat(
          "Network isn't in CIDR format");
    }
    this->value = T(info.bytes);
  }
};

}  // namespace detail

///@brief Binary formatter for utils::ip::NetworkV4
template <>
struct BufferFormatter<NetworkV4> : detail::NetworkBufferFormatter<NetworkV4> {
  using BaseType = detail::NetworkBufferFormatter<NetworkV4>;

  using BaseType::BaseType;
};

///@brief Binary formatter for utils::ip::NetworkV6
template <>
struct BufferFormatter<NetworkV6> : detail::NetworkBufferFormatter<NetworkV6> {
  using BaseType = detail::NetworkBufferFormatter<NetworkV6>;

  using BaseType::BaseType;
};

///@brief Binary formatter for utils::ip::AddressV4
template <>
struct BufferFormatter<AddressV4> : detail::AddressNetworkBuffer<AddressV4> {
  using BaseType = detail::AddressNetworkBuffer<AddressV4>;

  using BaseType::BaseType;
};

///@brief Binary formatter for utils::ip::AddressV6
template <>
struct BufferFormatter<AddressV6> : detail::AddressNetworkBuffer<AddressV6> {
  using BaseType = detail::AddressNetworkBuffer<AddressV6>;

  using BaseType::BaseType;
};

///@brief Binary formatter for utils::ip::InetNetwork
template <>
struct BufferFormatter<InetNetwork>
    : detail::IpBufferFormatterBase<InetNetwork> {
  using BaseType = detail::IpBufferFormatterBase<InetNetwork>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) {
    using Address = std::vector<unsigned char>;
    typename BaseType::template IpFormatterInfo<Address> info{
        /* .address = */ this->value.GetBytes(),
        /* .address_family = */
        (this->value.GetAddressFamily() == InetNetwork::AddressFamily::IPv4)
            ? detail::kPgsqlAfInet
            : detail::kPgsqlAfInet6,
        /* .prefix_length = */ static_cast<char>(this->value.GetPrefixLength()),
        /* .is_cidr = */ detail::kIsInet};
    BaseType::Format(info, types, buffer);
  }
};

/// @brief Binary parser for utils::ip::NetworkV4
template <>
struct BufferParser<NetworkV4> : detail::NetworkBufferParser<NetworkV4> {
  using BaseType = detail::NetworkBufferParser<NetworkV4>;

  using BaseType::BaseType;
};

/// @brief Binary parser for utils::ip::NetworkV6
template <>
struct BufferParser<NetworkV6> : detail::NetworkBufferParser<NetworkV6> {
  using BaseType = detail::NetworkBufferParser<NetworkV6>;

  using BaseType::BaseType;
};

/// @brief Binary parser for utils::ip::AddressV4
template <>
struct BufferParser<AddressV4> : detail::AddressBufferParser<AddressV4> {
  using BaseType = detail::AddressBufferParser<AddressV4>;

  using BaseType::BaseType;
};

/// @brief Binary parser for utils::ip::AddressV6
template <>
struct BufferParser<AddressV6> : detail::AddressBufferParser<AddressV6> {
  using BaseType = detail::AddressBufferParser<AddressV6>;

  using BaseType::BaseType;
};

/// @brief Binary parser for utils::ip::InetNetwork
template <>
struct BufferParser<InetNetwork> : detail::IpBufferParserBase<InetNetwork> {
  using BaseType = detail::IpBufferParserBase<InetNetwork>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    using Bytes = std::vector<unsigned char>;
    auto info = BaseType::template Parse<Bytes>(buffer);
    this->value = InetNetwork(std::move(info.bytes), info.prefix_length,
                              (info.family == detail::kPgsqlAfInet
                                   ? InetNetwork::AddressFamily::IPv4
                                   : InetNetwork::AddressFamily::IPv6));
  }
};

//@{
/** @name C++ to PostgreSQL mapping for ip types */
template <>
struct CppToSystemPg<NetworkV4> : PredefinedOid<PredefinedOids::kCidr> {};
template <>
struct CppToSystemPg<NetworkV6> : PredefinedOid<PredefinedOids::kCidr> {};
template <>
struct CppToSystemPg<AddressV6> : PredefinedOid<PredefinedOids::kCidr> {};
template <>
struct CppToSystemPg<AddressV4> : PredefinedOid<PredefinedOids::kCidr> {};
template <>
struct CppToSystemPg<InetNetwork> : PredefinedOid<PredefinedOids::kInet> {};
//@}

}  // namespace io

}  // namespace storages::postgres

USERVER_NAMESPACE_END
