#include <gtest/gtest.h>

#include <userver/utils/macaddr/macaddr8.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Macaddr8Test, Test) {
  using utils::macaddr::Macaddr8;
  using utils::macaddr::Macaddr8FromString;

  EXPECT_THROW(Macaddr8FromString(""), std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("g8:00:2b:01:02:03:05"),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("a08:00:2b:01:02:03:05"),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("08:00:2b:01:02-03:05"),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("08:00:2b:01:02:03:04:05:05"),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString(" 08:00:2b:01:02:03:05"),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("08:00:2b:01:02:03:05 "),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("0x08:0x00:0x2b:0x01:0x02:0x03:0x05"),
               std::invalid_argument);
  EXPECT_THROW(Macaddr8FromString("08~00~2b~01~02~03~05"),
               std::invalid_argument);

  Macaddr8::OctetsType octets = {0x08, 0x00, 0x2b, 0x01,
                                 0x02, 0x03, 0x04, 0x05};
  EXPECT_EQ(Macaddr8FromString("08:00:2b:01:02:03:04:05"), Macaddr8(octets));
  EXPECT_EQ(Macaddr8FromString("08-00-2b-01-02-03-04-05"), Macaddr8(octets));
  EXPECT_EQ(Macaddr8FromString("08.00.2b.01.02.03.04.05"), Macaddr8(octets));
  EXPECT_EQ(Macaddr8FromString("08002b0102030405"), Macaddr8(octets));
  EXPECT_EQ(Macaddr8FromString("08002b:0102030405"), Macaddr8(octets));
  EXPECT_EQ(Macaddr8FromString("08002b-0102030405"), Macaddr8(octets));
  EXPECT_EQ(Macaddr8FromString("08002b.0102030405"), Macaddr8(octets));

  // Macaddr compatability
  Macaddr8::OctetsType octets1 = {0x08, 0x00, 0x2b, 0xFF,
                                  0xFE, 0x03, 0x04, 0x05};
  EXPECT_EQ(Macaddr8FromString("08:00:2b:03:04:05"), Macaddr8(octets1));
  EXPECT_EQ(Macaddr8FromString("08-00-2b-03-04-05"), Macaddr8(octets1));
  EXPECT_EQ(Macaddr8FromString("08.00.2b.03.04.05"), Macaddr8(octets1));
  EXPECT_EQ(Macaddr8FromString("08002b030405"), Macaddr8(octets1));
  EXPECT_EQ(Macaddr8FromString("08002b-030405"), Macaddr8(octets1));
  EXPECT_EQ(Macaddr8FromString("08002b.030405"), Macaddr8(octets1));
  EXPECT_EQ(Macaddr8FromString("08002b:030405"), Macaddr8(octets1));
}

USERVER_NAMESPACE_END
