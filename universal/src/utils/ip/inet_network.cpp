#include <stdexcept>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/utils/ip/inet_network.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip {

template <typename T>
T CidrNetworkFromInetNetwork(const InetNetwork& inet_network) {
  typename T::AddressType::BytesType bytes;
  const auto& inet_bytes = inet_network.GetBytes();
  std::copy(inet_bytes.cbegin(), inet_bytes.cend(), bytes.begin());
  return T(typename T::AddressType(bytes), inet_network.GetPrefixLength());
}

template <typename T>
InetNetwork InetNetworkFromCidrNetwork(const T& network) {
  const auto bytes = network.GetAddress().GetBytes();
  std::vector<unsigned char> inet_bytes;
  inet_bytes.reserve(bytes.size());
  std::copy(bytes.cbegin(), bytes.cend(), std::back_inserter(inet_bytes));
  return InetNetwork(std::move(inet_bytes), network.GetPrefixLength(),
                     std::is_same_v<T, NetworkV4>
                         ? InetNetwork::AddressFamily::IPv4
                         : InetNetwork::AddressFamily::IPv6);
}

InetNetwork::InetNetwork()
    : prefix_length_(32), address_family_(AddressFamily::IPv4) {
  bytes_.resize(4);
}

InetNetwork::InetNetwork(std::vector<unsigned char>&& bytes,
                         unsigned char prefix_length,
                         AddressFamily address_family)
    : bytes_(std::move(bytes)),
      prefix_length_(prefix_length),
      address_family_(address_family) {
  if (!(bytes_.size() == 4 && prefix_length_ <= 32 &&
        address_family_ == AddressFamily::IPv4) &&
      !(bytes_.size() == 16 && prefix_length_ <= 128 &&
        address_family_ == AddressFamily::IPv6)) {
    throw std::invalid_argument("Invalid IP address format");
  }
}

NetworkV4 NetworkV4FromInetNetwork(const InetNetwork& inet_network) {
  return CidrNetworkFromInetNetwork<NetworkV4>(inet_network);
}

NetworkV6 NetworkV6FromInetNetwork(const InetNetwork& inet_network) {
  return CidrNetworkFromInetNetwork<NetworkV6>(inet_network);
}

InetNetwork NetworkV4ToInetNetwork(const NetworkV4& network) {
  return InetNetworkFromCidrNetwork(network);
}

InetNetwork NetworkV6ToInetNetwork(const NetworkV6& network) {
  return InetNetworkFromCidrNetwork(network);
}

}  // namespace utils::ip

USERVER_NAMESPACE_END
