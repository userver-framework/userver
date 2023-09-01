#pragma once

/// @file userver/utils/ip/network_base.hpp
/// @brief @copybrief utils::ip::NetworkBase

#include <fmt/format.h>

#include <userver/utils/ip/address_v4.hpp>
#include <userver/utils/ip/address_v6.hpp>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip::detail {

template <typename T>
inline constexpr bool kIsAddressType =
    std::is_same_v<T, AddressV4> || std::is_same_v<T, AddressV6>;

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

}  // namespace utils::ip::detail

USERVER_NAMESPACE_END
