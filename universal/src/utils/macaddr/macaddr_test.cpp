#include <gtest/gtest.h>

#include <userver/utils/macaddr/macaddr.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MacaddrTest, FromStringTests) {
  using userver::utils::macaddr::Macaddr;
  using userver::utils::macaddr::MacaddrFromString;

  EXPECT_THROW(MacaddrFromString(""), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("g8:00:2b:01:02:03"), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("a08:00:2b:01:02:03"), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("08:00:2b:01:02-03"), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("08:00:2b:01:02:03:04"), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString(" 08:00:2b:01:02:03"), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("08:00:2b:01:02:03 "), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("0x08:0x00:0x2b:0x01:0x02:0x03"), std::invalid_argument);
  EXPECT_THROW(MacaddrFromString("08~00~2b~01~02~03"), std::invalid_argument);

  Macaddr::OctetsType octets = {0x08, 0x00, 0x2b, 0x01, 0x02, 0x03};
  EXPECT_EQ(MacaddrFromString("08:00:2b:01:02:03"), Macaddr(octets));
  EXPECT_EQ(MacaddrFromString("08-00-2b-01-02-03"), Macaddr(octets));
  EXPECT_EQ(MacaddrFromString("08.00.2b.01.02.03"), Macaddr(octets));
  EXPECT_EQ(MacaddrFromString("08002b010203"), Macaddr(octets));
  EXPECT_EQ(MacaddrFromString("08002b:010203"), Macaddr(octets));
  EXPECT_EQ(MacaddrFromString("08002b-010203"), Macaddr(octets));
  EXPECT_EQ(MacaddrFromString("08002b.010203"), Macaddr(octets));
}

TEST(MacaddrTest, ToStringTests) {
  using userver::utils::macaddr::Macaddr;
  using userver::utils::macaddr::MacaddrToString;

  Macaddr::OctetsType octets = {0x08, 0x00, 0x2b, 0x01, 0x02, 0x03};
  EXPECT_EQ(MacaddrToString(Macaddr(octets)), "08:00:2b:01:02:03");
}

USERVER_NAMESPACE_END
