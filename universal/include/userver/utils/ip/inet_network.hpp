#pragma once

/// @file userver/utils/ip/inet_network.hpp
/// @brief @copybrief utils::ip::InetNetwork

#include <vector>

#include <sys/socket.h>

#include <userver/utils/ip/network_v4.hpp>
#include <userver/utils/ip/network_v6.hpp>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

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

}  // namespace utils::ip

USERVER_NAMESPACE_END
