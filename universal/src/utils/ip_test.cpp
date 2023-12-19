#include <gtest/gtest.h>

#include <userver/utils/ip.hpp>

USERVER_NAMESPACE_BEGIN

using utils::ip::AddressV4;
using utils::ip::AddressV6;
using utils::ip::InetNetwork;
using utils::ip::NetworkV4;
using utils::ip::NetworkV6;

TEST(AddressV4Test, ToStringTests) {
  using utils::ip::AddressV4ToString;

  AddressV4 a1;
  EXPECT_EQ(AddressV4ToString(a1), "0.0.0.0");

  AddressV4::BytesType b1 = {{1, 2, 3, 4}};
  AddressV4 a2(b1);
  EXPECT_EQ(AddressV4ToString(a2), "1.2.3.4");
}

TEST(AddressV4Test, FromStringTests) {
  using utils::ip::AddressSystemError;
  using utils::ip::AddressV4FromString;

  EXPECT_THROW(AddressV4FromString("abcdef"), AddressSystemError);
  EXPECT_THROW(AddressV4FromString(""), AddressSystemError);
  EXPECT_THROW(AddressV4FromString("256.1.1.1"), AddressSystemError);
  EXPECT_THROW(AddressV4FromString("1.256.1.1"), AddressSystemError);
  EXPECT_THROW(AddressV4FromString("1.1.256.1"), AddressSystemError);
  EXPECT_THROW(AddressV4FromString("1.1.1.256"), AddressSystemError);
  EXPECT_THROW(AddressV4FromString(" 1.1.1.1"), AddressSystemError);
  EXPECT_THROW(AddressV4FromString("1.1.1.1 "), AddressSystemError);

  EXPECT_EQ(AddressV4FromString("0.0.0.0"), AddressV4());
  EXPECT_EQ(AddressV4FromString("1.2.3.4"), AddressV4({1, 2, 3, 4}));
}

TEST(AddressV6Test, ToStringTests) {
  using utils::ip::AddressV6;
  using utils::ip::AddressV6ToString;

  AddressV6::BytesType b1{0};
  b1[0] = 0xFF;
  b1[1] = 0xFF;
  EXPECT_EQ(AddressV6ToString(AddressV6(b1)), "ffff::");

  AddressV6::BytesType b2{0};
  b2[14] = 0xFF;
  b2[15] = 0xFF;
  EXPECT_EQ(AddressV6ToString(AddressV6(b2)), "::ffff");
}

TEST(AddressV6Test, FromStringTests) {
  using utils::ip::AddressV6;
  using utils::ip::AddressV6FromString;

  EXPECT_THROW(AddressV6FromString("abcdef"), utils::ip::AddressSystemError);
  EXPECT_THROW(AddressV6FromString(""), utils::ip::AddressSystemError);
  EXPECT_THROW(AddressV6FromString("gf::1"), utils::ip::AddressSystemError);

  AddressV6::BytesType b1{0};
  b1[0] = 0xFF;
  b1[1] = 0xFF;
  EXPECT_EQ(AddressV6FromString("ffff::"), AddressV6(b1));

  AddressV6::BytesType b2{0};
  b2[14] = 0xFF;
  b2[15] = 0xFF;
  EXPECT_EQ(AddressV6FromString("::ffff"), AddressV6(b2));
}

TEST(NetworkV4Test, ConstructorTest) {
  AddressV4 addr({1, 2, 3, 4});
  EXPECT_THROW(NetworkV4(addr, 33), std::out_of_range);
}

TEST(NetworkV4Test, FromStringTests) {
  using utils::ip::NetworkV4FromString;

  EXPECT_THROW(NetworkV4FromString("1.1.1.256/24"),
               utils::ip::AddressSystemError);
  EXPECT_THROW(NetworkV4FromString("1.1.1.1/33"), std::invalid_argument);
  EXPECT_THROW(NetworkV4FromString("1.1.1.1/-1"), std::invalid_argument);
  EXPECT_THROW(NetworkV4FromString("1.1.1.1"), std::invalid_argument);
  EXPECT_THROW(NetworkV4FromString("abcdef/24"), utils::ip::AddressSystemError);
  EXPECT_THROW(NetworkV4FromString("1.1.1.1/"), std::invalid_argument);
  EXPECT_THROW(NetworkV4FromString("1.1.1.1/abc"), std::invalid_argument);

  EXPECT_EQ(NetworkV4FromString("1.1.1.1/24"),
            NetworkV4(AddressV4({1, 1, 1, 1}), 24));
  EXPECT_EQ(NetworkV4FromString("0.0.0.0/32"), NetworkV4(AddressV4(), 32));
}

TEST(NetworkV4Test, ToStringTests) {
  using utils::ip::NetworkV4ToString;
  EXPECT_EQ(NetworkV4ToString(NetworkV4(AddressV4({192, 168, 1, 1}), 24)),
            "192.168.1.1/24");
  EXPECT_EQ(NetworkV4ToString(NetworkV4(AddressV4(), 32)), "0.0.0.0/32");
}

TEST(NetworkV4Test, TransformToCidrFormatTest) {
  using utils::ip::TransformToCidrFormat;

  const auto net1 = NetworkV4(AddressV4({255, 255, 255, 1}), 24);
  EXPECT_EQ(TransformToCidrFormat(net1),
            NetworkV4(AddressV4({255, 255, 255, 0}), 24));
  const auto net2 = NetworkV4(AddressV4({255, 255, 1, 1}), 16);
  EXPECT_EQ(TransformToCidrFormat(net2),
            NetworkV4(AddressV4({255, 255, 0, 0}), 16));
  const auto net3 = NetworkV4(AddressV4({255, 1, 1, 1}), 8);
  EXPECT_EQ(TransformToCidrFormat(net3),
            NetworkV4(AddressV4({255, 0, 0, 0}), 8));
  const auto net4 = NetworkV4(AddressV4({1, 1, 1, 1}), 0);
  EXPECT_EQ(TransformToCidrFormat(net4), NetworkV4(AddressV4(), 0));
  const auto net5 = NetworkV4(AddressV4({255, 255, 255, 255}), 28);
  EXPECT_EQ(TransformToCidrFormat(net5),
            NetworkV4(AddressV4({255, 255, 255, 240}), 28));
  const auto net6 = NetworkV4(AddressV4({255, 255, 255, 255}), 25);
  EXPECT_EQ(TransformToCidrFormat(net6),
            NetworkV4(AddressV4({255, 255, 255, 128}), 25));
  const auto net7 = NetworkV4(AddressV4({255, 255, 255, 1}), 20);
  EXPECT_EQ(TransformToCidrFormat(net7),
            NetworkV4(AddressV4({255, 255, 240, 0}), 20));
}

TEST(NetworkV6Test, PrefixLengthTests) {
  using utils::ip::AddressV6;
  using utils::ip::NetworkV6;

  AddressV6::BytesType b1;
  b1[0] = 0xFF;
  b1[1] = 0xFF;
  EXPECT_THROW(NetworkV6(AddressV6({b1}), 129), std::out_of_range);
}

TEST(NetworkV6Test, NetworkFromString) {
  using utils::ip::AddressV6;
  using utils::ip::NetworkV6;
  using utils::ip::NetworkV6FromString;

  EXPECT_THROW(NetworkV6FromString("a:b/24"), utils::ip::AddressSystemError);
  EXPECT_THROW(NetworkV6FromString("2001:370::10:7344/129"),
               std::invalid_argument);
  EXPECT_THROW(NetworkV6FromString("2001:370::10:7344/-1"),
               std::invalid_argument);
  EXPECT_THROW(NetworkV6FromString("2001:370::10:7344/"),
               std::invalid_argument);
  EXPECT_THROW(NetworkV6FromString("2001:370::10:7344"), std::invalid_argument);

  AddressV6::BytesType b1{0};
  b1[0] = 0xFF;
  b1[1] = 0xFF;
  EXPECT_EQ(NetworkV6FromString("ffff::/128"), NetworkV6(AddressV6(b1), 128));

  AddressV6::BytesType b2{0};
  b2[14] = 0xFF;
  b2[15] = 0xFF;
  EXPECT_EQ(NetworkV6FromString("::ffff/128"), NetworkV6(AddressV6(b2), 128));
}

TEST(NetworkV6Test, NetworkToString) {
  using utils::ip::AddressV6;
  using utils::ip::NetworkV6;
  using utils::ip::NetworkV6ToString;

  AddressV6::BytesType b1{0};
  b1[0] = 0xFF;
  b1[1] = 0xFF;
  EXPECT_EQ(NetworkV6ToString(NetworkV6(AddressV6(b1), 128)), "ffff::/128");

  AddressV6::BytesType b2{0};
  b2[14] = 0xFF;
  b2[15] = 0xFF;
  EXPECT_EQ(NetworkV6ToString(NetworkV6(AddressV6(b2), 128)), "::ffff/128");
}

TEST(NetworkV6Test, TransformToCidrFormatTest) {
  using utils::ip::NetworkV6FromString;
  using utils::ip::TransformToCidrFormat;

  const auto net1 = NetworkV6FromString("2001:db8:85a3::8a2e:370:7334/128");
  EXPECT_EQ(TransformToCidrFormat(net1),
            NetworkV6FromString("2001:db8:85a3::8a2e:370:7334/128"));
  const auto net2 = NetworkV6FromString("2001:db8:85a3::8a2e:370:7334/120");
  EXPECT_EQ(TransformToCidrFormat(net2),
            NetworkV6FromString("2001:db8:85a3::8a2e:370:7300/120"));
}

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
