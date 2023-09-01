#include <userver/utils/ip/inet_network.hpp>
#include <userver/utils/ip/network_v4.hpp>
#include <userver/utils/ip/network_v6.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

using utils::ip::AddressV4;
using utils::ip::AddressV6;
using utils::ip::InetNetwork;
using utils::ip::NetworkV4;
using utils::ip::NetworkV6;

TEST(InetNetworkTest, ConstructorTests) {
  // Invalid IPv4 bytes size
  std::vector<unsigned char> inet_v4_invalid_bytes(0, 5);
  EXPECT_THROW(InetNetwork(std::move(inet_v4_invalid_bytes), 32,
                           InetNetwork::AddressFamily::IPv4),
               std::invalid_argument);

  // Invalid IPv4 prefix length
  std::vector<unsigned char> inet_v4_bytes(0, 4);
  EXPECT_THROW(InetNetwork(std::move(inet_v4_bytes), 33,
                           InetNetwork::AddressFamily::IPv4),
               std::invalid_argument);

  // Invalid IPv6 bytes size
  std::vector<unsigned char> inet_v6_invalid_bytes(0, 17);
  EXPECT_THROW(InetNetwork(std::move(inet_v6_invalid_bytes), 128,
                           InetNetwork::AddressFamily::IPv6),
               std::invalid_argument);

  // Invalid IPv4 prefix length
  std::vector<unsigned char> inet_v6(0, 16);
  EXPECT_THROW(
      InetNetwork(std::move(inet_v6), 129, InetNetwork::AddressFamily::IPv6),
      std::invalid_argument);
}

TEST(InetNetworkTest, ToInetNetworkTests) {
  using utils::ip::NetworkV4ToInetNetwork;
  using utils::ip::NetworkV6ToInetNetwork;

  NetworkV4 net_v4(AddressV4({127, 0, 0, 1}), 32);
  InetNetwork inet_v4({127, 0, 0, 1}, 32, InetNetwork::AddressFamily::IPv4);
  const auto res_inet_v4 = NetworkV4ToInetNetwork(net_v4);
  EXPECT_EQ(res_inet_v4, inet_v4);

  NetworkV6 net_v6(AddressV6({0}), 120);
  std::vector<unsigned char> inet_v6_bytes(16, 0);
  InetNetwork inet_v6(std::move(inet_v6_bytes), 120,
                      InetNetwork::AddressFamily::IPv6);
  const auto res_inet_v6 = NetworkV6ToInetNetwork(net_v6);
  EXPECT_EQ(res_inet_v6, inet_v6);
}

TEST(InetNetworkTest, FromInetNetworkTests) {
  using utils::ip::NetworkV4FromInetNetwork;
  using utils::ip::NetworkV6FromInetNetwork;

  NetworkV4 net_v4(AddressV4({127, 0, 0, 1}), 32);
  InetNetwork inet_v4({127, 0, 0, 1}, 32, InetNetwork::AddressFamily::IPv4);
  const auto res_net_v4 = NetworkV4FromInetNetwork(inet_v4);
  EXPECT_EQ(res_net_v4, net_v4);

  NetworkV6 net_v6(AddressV6(), 128);
  InetNetwork inet_v6(std::vector<unsigned char>(16, 0), 128,
                      InetNetwork::AddressFamily::IPv6);
  EXPECT_EQ(NetworkV6FromInetNetwork(inet_v6), net_v6);
}

USERVER_NAMESPACE_END
