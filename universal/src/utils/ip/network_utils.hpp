#pragma once

#include "address_utils.hpp"

#include <stdexcept>
#include <string>

#include <fmt/format.h>

#include <userver/utils/from_string.hpp>
#include <userver/utils/ip/address_v4.hpp>
#include <userver/utils/ip/address_v6.hpp>
#include <userver/utils/ip/network_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip::detail {

///@brief Convert Network IPv4/IPv6 from std::string_view
template <typename Address,
          typename = std::enable_if_t<kIsAddressType<Address>>>
NetworkBase<Address> NetworkFromString(const std::string& str) {
  const auto throw_exception = []() {
    throw std::invalid_argument(fmt::format(
        "Error while converting {} to string",
        std::is_same_v<Address, AddressV4> ? "NetworkV4" : "NetworkV6"));
  };
  auto pos = str.find_first_of('/');
  if ((pos == std::string::npos) || (pos == str.size() - 1)) {
    throw_exception();
  }
  auto end = str.find_first_not_of("0123456789", pos + 1);
  if (end != std::string::npos) {
    throw_exception();
  }
  const auto addr =
      detail::AddressFromString<Address::AddressSize>(str.substr(0, pos));
  const int prefix_len = utils::FromString<int>(str.substr(pos + 1));
  if (prefix_len < 0 ||
      prefix_len > NetworkBase<Address>::kMaximumPrefixLength) {
    throw_exception();
  }
  return NetworkBase<Address>(addr, static_cast<unsigned char>(prefix_len));
}

///@brief Convert Network IPv4/IPv6 to std::string
template <typename Address>
NetworkBase<Address> TransformToCidrNetwork(NetworkBase<Address> address) {
  auto bytes = address.GetAddress().GetBytes();
  const auto prefix_length = address.GetPrefixLength();
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    if (prefix_length <= i * 8) {
      bytes[i] = 0;
    } else if (prefix_length < (i + 1) * 8) {
      bytes[i] &= 0xFF00 >> (prefix_length % 8);
    }
  }
  return NetworkBase<Address>(typename NetworkBase<Address>::AddressType(bytes),
                              prefix_length);
}

}  // namespace utils::ip::detail

USERVER_NAMESPACE_END
