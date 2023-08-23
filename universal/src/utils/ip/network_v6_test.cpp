#include <gtest/gtest.h>

#include <userver/utils/ip/exception.hpp>
#include <userver/utils/ip/network_v6.hpp>

USERVER_NAMESPACE_BEGIN

TEST(NetworkV6Test, PrefixLengthTests) {
  using namespace utils::ip;

  AddressV6::BytesType b1;
  b1[0] = 0xFF;
  b1[1] = 0xFF;
  EXPECT_THROW(NetworkV6(AddressV6({b1}), 129), std::out_of_range);
}

TEST(NetworkV6Test, NetworkFromString) {
  using namespace utils::ip;

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
  using namespace utils::ip;

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
  using namespace utils::ip;

  const auto net1 = NetworkV6FromString("2001:db8:85a3::8a2e:370:7334/128");
  EXPECT_EQ(TransformToCidrFormat(net1), NetworkV6FromString("2001:db8:85a3::8a2e:370:7334/128"));
  const auto net2 = NetworkV6FromString("2001:db8:85a3::8a2e:370:7334/120");
  EXPECT_EQ(TransformToCidrFormat(net2), NetworkV6FromString("2001:db8:85a3::8a2e:370:7300/120"));
}

USERVER_NAMESPACE_END
