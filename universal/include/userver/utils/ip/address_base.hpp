#pragma once

/// @file userver/utils/ip/address_base.hpp
/// @brief @copybrief utils::ip::AddressBase

#include <array>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip::detail {

template <std::size_t N>
constexpr bool kIsValidAddressSize = (N == 4 || N == 16);

/// @ingroup userver_containers
///
/// @brief Base class for IPv4/IPv6 addresses
template <std::size_t N, typename = std::enable_if_t<kIsValidAddressSize<N>>>
class AddressBase final {
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

}  // namespace utils::ip::detail

USERVER_NAMESPACE_END
