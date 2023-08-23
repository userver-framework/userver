#include <userver/utils/ip/address_v4.hpp>
#include "userver/utils/ip/exception.hpp"
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(AddressV4Test, ToStringTests) {
  using namespace utils::ip;

  AddressV4 a1;
  EXPECT_EQ(AddressV4ToString(a1), "0.0.0.0");
  
  AddressV4::BytesType b1 = {{1, 2, 3, 4}};
  AddressV4 a2(b1);
  EXPECT_EQ(AddressV4ToString(a2), "1.2.3.4");
}

TEST(AddressV4Test, FromStringTests) {
  using namespace utils::ip;

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
