#include <gtest/gtest.h>

#include <userver/utils/ip/address_v4.hpp>
#include <userver/utils/ip/exception.hpp>

USERVER_NAMESPACE_BEGIN

using utils::ip::AddressV4;

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

USERVER_NAMESPACE_END
