#pragma once

/// @file userver/utils/macaddr/macaddr_base.hpp
/// @brief MAC address types

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief Base class for Macaddr/Macaddr8
template <std::size_t N>
class MacaddrBase final {
  static_assert(N == 6 || N == 8, "Address can only be 6 or 8 bytes size");

 public:
  using OctetsType = std::array<unsigned char, N>;

  MacaddrBase() = default;

  explicit MacaddrBase(const OctetsType& macaddr) noexcept
      : macaddr_(macaddr) {}

  /// @brief Get octets of MAC-address
  const OctetsType& GetOctets() const noexcept { return macaddr_; }

  friend bool operator==(const MacaddrBase<N>& a,
                         const MacaddrBase<N>& b) noexcept {
    return a.GetOctets() == b.GetOctets();
  }

  friend bool operator!=(const MacaddrBase<N>& a,
                         const MacaddrBase<N>& b) noexcept {
    return !(a == b);
  }

 private:
  OctetsType macaddr_ = {0};
};

/// @ingroup userver_containers
///
/// @brief 48-bit MAC address
using Macaddr = MacaddrBase<6>;

/// @ingroup userver_containers
///
/// @brief 64-bit MAC address
using Macaddr8 = MacaddrBase<8>;

/// @brief Get 48-bit MAC address as a string in "xx:xx:.." format.
std::string MacaddrToString(Macaddr macaddr);

/// @brief Get 64-bit MAC address as a string in "xx:xx:.." format.
std::string Macaddr8ToString(Macaddr8 macaddr);

/// @brief Get 48-bit MAC address from std::string.
Macaddr MacaddrFromString(const std::string& str);

/// @brief Get 64-bit MAC address from std::string.
Macaddr8 Macaddr8FromString(const std::string& str);

}  // namespace utils

USERVER_NAMESPACE_END
