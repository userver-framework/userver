#pragma once

/// @file userver/utils/ip.hpp
/// @brief IPv4 and IPv6 addresses and networks

#include <array>
#include <exception>
#include <string>
#include <system_error>
#include <vector>

#include <sys/socket.h>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

/// @ingroup userver_containers
///
/// @brief Base class for IPv4/IPv6 addresses
template <std::size_t N>
class AddressBase final {
  static_assert(N == 4 || N == 16, "Address can only be 4 or 16 bytes size");

 public:
  static constexpr std::size_t AddressSize = N;
  using BytesType = std::array<unsigned char, N>;

  AddressBase() noexcept : address_({0}) {}
  explicit AddressBase(const BytesType& address) : address_(address) {}

  /// @brief Get the address in bytes, in network byte order.
  const BytesType& GetBytes() const noexcept { return address_; }

  friend bool operator==(const AddressBase<N>& a1,
                         const AddressBase<N>& a2) noexcept {
    return a1.address_ == a2.address_;
  }

  friend bool operator!=(const AddressBase<N>& a1,
                         const AddressBase<N>& a2) noexcept {
    return a1.address_ != a2.address_;
  }

 private:
  BytesType address_;
};

/// @ingroup userver_containers
///
/// @brief IPv4 address in network bytes order
using AddressV4 = AddressBase<4>;

/// @ingroup userver_containers
///
/// @brief IPv6 address in network bytes order
using AddressV6 = AddressBase<16>;

template <typename T>
inline constexpr bool kIsAddressType =
    std::is_same_v<T, AddressV4> || std::is_same_v<T, AddressV6>;

/// @brief Create an IPv4 address from an IP address string in dotted decimal
/// form.
/// @throw AddressSystemError
AddressV4 AddressV4FromString(const std::string& str);

/// @brief Create an IPv6 address from an IP address string in dotted decimal
/// form.
AddressV6 AddressV6FromString(const std::string& str);

/// @brief Get the address as a string in dotted decimal format.
std::string AddressV4ToString(const AddressV4& address);

/// @brief Get the address as a string in dotted decimal format.
std::string AddressV6ToString(const AddressV6& address);

/// @ingroup userver_containers
///
/// @brief Base class for IPv4/IPv6 network
template <typename Address,
          typename = std::enable_if_t<kIsAddressType<Address>>>
class NetworkBase final {
 public:
  using AddressType = Address;
  static constexpr unsigned char kMaximumPrefixLength =
      std::is_same_v<Address, AddressV4> ? 32 : 128;

  NetworkBase() noexcept = default;

  NetworkBase(const AddressType& address, unsigned short prefix_length)
      : address_(address), prefix_length_(prefix_length) {
    if (prefix_length > kMaximumPrefixLength) {
      throw std::out_of_range(fmt::format(
          "{} prefix length is too large",
          std::is_same_v<Address, AddressV4> ? "NetworkV4" : "NetworkV6"));
    }
  }

  /// @brief Get the address address of network
  AddressType GetAddress() const noexcept { return address_; }

  /// @brief Get prefix length of address network
  unsigned char GetPrefixLength() const noexcept { return prefix_length_; }

  friend bool operator==(const NetworkBase<Address>& a,
                         const NetworkBase<Address>& b) noexcept {
    return a.address_ == b.address_ && a.prefix_length_ == b.prefix_length_;
  }

  friend bool operator!=(const NetworkBase<Address>& a,
                         const NetworkBase<Address>& b) noexcept {
    return !(a == b);
  }

 private:
  AddressType address_;
  unsigned char prefix_length_ = 0;
};

/// @ingroup userver_containers
///
/// @brief IPv4 network.
using NetworkV4 = NetworkBase<AddressV4>;

/// @ingroup userver_containers
///
/// @brief IPv6 network.
using NetworkV6 = NetworkBase<AddressV6>;

///@brief Create an IPv4 network from a string containing IP address and prefix
/// length.
/// @throw std::invalid_argument, AddressSystemError
NetworkV4 NetworkV4FromString(const std::string& str);

/// @brief Create an IPv6 network from a string containing IP address and prefix
/// length.
NetworkV6 NetworkV6FromString(const std::string& str);

///@brief Get the network as an address in dotted decimal format.
std::string NetworkV4ToString(const NetworkV4& network);

/// @brief Get the network as an address in dotted decimal format.
std::string NetworkV6ToString(const NetworkV6& network);

/// @brief Convert NetworkV4 to CIDR format
NetworkV4 TransformToCidrFormat(NetworkV4 network);

/// @brief Convert NetworkV4 to CIDR format
NetworkV6 TransformToCidrFormat(NetworkV6 network);

/// @ingroup userver_containers
///
/// @brief INET IPv4/IPv4 network
/// @warning InetNetwork class is deprecated. You should use InetNetwork class
/// via transformation function to/from NetworkV4/NetworkV6.
/// Use this class only if you need to work with INET PostgreSQL format.
class InetNetwork final {
 public:
  enum class AddressFamily : unsigned char { IPv4 = AF_INET, IPv6 = AF_INET6 };

  // Default constructor: IPv4 address
  InetNetwork();
  InetNetwork(std::vector<unsigned char>&& bytes, unsigned char prefix_length,
              AddressFamily address_family);

  /// @brief Get the address in bytes
  const std::vector<unsigned char>& GetBytes() const noexcept { return bytes_; }

  /// @brief Get the prefix length of network
  unsigned char GetPrefixLength() const noexcept { return prefix_length_; }

  /// @brief Get the address family
  AddressFamily GetAddressFamily() const noexcept { return address_family_; }

  friend bool operator==(const InetNetwork& lhs, const InetNetwork& rhs) {
    return lhs.address_family_ == rhs.address_family_ &&
           lhs.prefix_length_ == rhs.prefix_length_ && lhs.bytes_ == rhs.bytes_;
  }

  friend bool operator!=(const InetNetwork& lhs, const InetNetwork& rhs) {
    return !operator==(lhs, rhs);
  }

 private:
  std::vector<unsigned char> bytes_;
  unsigned char prefix_length_;
  AddressFamily address_family_;
};

/// @brief Convert InetNetwork to NetworkV4
NetworkV4 NetworkV4FromInetNetwork(const InetNetwork& inet_network);

/// @brief Convert InetNetwork to NetworkV6
NetworkV6 NetworkV6FromInetNetwork(const InetNetwork& inet_network);

/// @brief Convert NetworkV4 to InetNetwork
InetNetwork NetworkV4ToInetNetwork(const NetworkV4& network);

/// @brief Convert NetworkV6 to InetNetwork
InetNetwork NetworkV6ToInetNetwork(const NetworkV6& network);

/// @brief Invalid network or address
class AddressSystemError final : public std::exception {
 public:
  AddressSystemError(std::error_code code, std::string_view msg)
      : msg_(msg), code_(code) {}

  /// Operating system error code.
  const std::error_code& Code() const { return code_; }

  const char* what() const noexcept final { return msg_.c_str(); }

 private:
  std::string msg_;
  std::error_code code_;
};

}  // namespace utils::ip

USERVER_NAMESPACE_END
