
#include <stdexcept>

#include <gtest/gtest.h>

#include <userver/utils/ip/exception.hpp>
#include <userver/utils/ip/network_v4.hpp>

USERVER_NAMESPACE_BEGIN

using utils::ip::AddressV4;
using utils::ip::NetworkV4;

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

USERVER_NAMESPACE_END
