#pragma once

/// @file userver/utils/macaddr/macaddr_base.hpp
/// @brief @copybrief utils::macaddr::MacaddrBase

#include <array>
#include <stdexcept>
#include <utility>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::macaddr {

namespace detail {
template <std::size_t N>
inline constexpr bool kIsValidMacAddr = N == 6 || N == 8;
}

/// @ingroup userver_containers
///
/// @brief Base class for Macaddr/Macaddr8
template <std::size_t N,
          typename = std::enable_if_t<detail::kIsValidMacAddr<N>>>
class MacaddrBase final {
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

}  // namespace utils::macaddr

USERVER_NAMESPACE_END
