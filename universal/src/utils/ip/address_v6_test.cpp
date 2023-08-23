#include <cstddef>

#include <gtest/gtest.h>

#include <userver/utils/ip/address_v6.hpp>
#include "userver/utils/ip/exception.hpp"

USERVER_NAMESPACE_BEGIN

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

USERVER_NAMESPACE_END
