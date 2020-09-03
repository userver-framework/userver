#include <utils/decimal64.hpp>

#include <sstream>

#include <gtest/gtest.h>

using Dec4 = decimal64::decimal<4>;

TEST(Decimal64, ConstructFromString) {
  ASSERT_EQ(Dec4{"10"}, Dec4{10});
  ASSERT_EQ(Dec4{"10."}, Dec4{10});
  ASSERT_EQ(Dec4{"+10"}, Dec4{10});
  ASSERT_EQ(Dec4{"-10"}, Dec4{-10});

  ASSERT_EQ(Dec4{"-0"}, Dec4{0});
  ASSERT_EQ(Dec4{"+.0"}, Dec4{0});
  ASSERT_EQ(Dec4{"-.0"}, Dec4{0});
  ASSERT_EQ(Dec4{"010"}, Dec4{10});
  ASSERT_EQ(Dec4{"000005"}, Dec4{5});

  ASSERT_EQ(Dec4{"000000000000000000000000000000012.0"}, Dec4{12});
  ASSERT_EQ(Dec4{"000000000000000000000000000000000.12"}, Dec4{0.12});
  ASSERT_EQ(Dec4{"12345678987654.3210"}.AsUnbiased(), 123456789876543210LL);

  ASSERT_EQ(decimal64::decimal<18>{"0.987654321123456789"}.AsUnbiased(),
            987654321123456789LL);
  ASSERT_EQ(decimal64::decimal<18>{"0.0000000000000000126"}.AsUnbiased(), 13);
  ASSERT_EQ(decimal64::decimal<18>{"0.0000000000000000125"}.AsUnbiased(), 13);
  ASSERT_EQ(decimal64::decimal<18>{"0.0000000000000000124"}.AsUnbiased(), 12);
}

TEST(Decimal64, ConstructFromStringDeprecated) {
  // Deprecated: extra characters adjacent to the decimal
  // These will throw in a future revision
  ASSERT_EQ(Dec4{"123ab"}, Dec4{123});
  ASSERT_EQ(Dec4{"1#.1234"}, Dec4{1});
  ASSERT_EQ(Dec4{"12#.1234"}, Dec4{12});
  ASSERT_EQ(Dec4{"10.#123"}, Dec4{10});
  ASSERT_EQ(Dec4{"10.123#"}, Dec4{10.123});
  ASSERT_EQ(Dec4{"0x10"}, Dec4{0});
  ASSERT_EQ(Dec4{"+0x10"}, Dec4{0});
  ASSERT_EQ(Dec4{"-0x10"}, Dec4{0});
  ASSERT_EQ(Dec4{"0x1a"}, Dec4{0});
  ASSERT_EQ(Dec4{"+0x1a"}, Dec4{0});
  ASSERT_EQ(Dec4{"-0x1a"}, Dec4{0});

  // Deprecated: extra tokens after the decimal
  // Use stream operations instead
  // These will throw in a future revision
  ASSERT_EQ(Dec4{"   \t  \r-10. \n  ab"}, Dec4{-10});
  ASSERT_EQ(Dec4{"1 0"}, Dec4{1});
  ASSERT_EQ(Dec4{"+1 0"}, Dec4{1});
  ASSERT_EQ(Dec4{"-1 0"}, Dec4{-1});
  ASSERT_EQ(Dec4{"1. 0"}, Dec4{1});
  ASSERT_EQ(Dec4{"+1. 0"}, Dec4{1});
  ASSERT_EQ(Dec4{"-1. 0"}, Dec4{-1});
  ASSERT_EQ(Dec4{"1 .0"}, Dec4{1});
  ASSERT_EQ(Dec4{"+1 .0"}, Dec4{1});
  ASSERT_EQ(Dec4{"-1 .0"}, Dec4{-1});

  // Deprecated: parsing an invalid number results in 0
  // These will throw in a future revision
  ASSERT_EQ(Dec4{".+0"}, Dec4{0});
  ASSERT_EQ(Dec4{".-0"}, Dec4{0});
  ASSERT_EQ(Dec4{"#"}, Dec4{0});
  ASSERT_EQ(Dec4{"1234567898765432.1012"}, Dec4{0});
  ASSERT_EQ(Dec4{"99999999999999999999999999"}, Dec4{0});
  ASSERT_EQ(Dec4{"+99999999999999999999999999"}, Dec4{0});
  ASSERT_EQ(Dec4{"-99999999999999999999999999"}, Dec4{0});
}

TEST(Decimal64, FromString) {
  Dec4 out;

  ASSERT_TRUE(decimal64::fromString("1234.5678", out));
  ASSERT_EQ(out, Dec4{1234.5678});
  ASSERT_EQ(decimal64::fromString<Dec4>("1234.5678"), Dec4{1234.5678});

  ASSERT_FALSE(decimal64::fromString(".+0", out));
  ASSERT_FALSE(decimal64::fromString(".-0", out));
  ASSERT_FALSE(decimal64::fromString("1 0", out));
  ASSERT_FALSE(decimal64::fromString("+1 0", out));
  ASSERT_FALSE(decimal64::fromString("-1 0", out));
  ASSERT_FALSE(decimal64::fromString("1. 0", out));
  ASSERT_FALSE(decimal64::fromString("+1. 0", out));
  ASSERT_FALSE(decimal64::fromString("-1. 0", out));
  ASSERT_FALSE(decimal64::fromString("1 .0", out));
  ASSERT_FALSE(decimal64::fromString("+1 .0", out));
  ASSERT_FALSE(decimal64::fromString("-1 .0", out));
  ASSERT_FALSE(decimal64::fromString("0x10", out));
  ASSERT_FALSE(decimal64::fromString("+0x10", out));
  ASSERT_FALSE(decimal64::fromString("-0x10", out));
  ASSERT_FALSE(decimal64::fromString("+0x1a", out));

  out = Dec4{1};
  ASSERT_FALSE(decimal64::fromString("", out));
  ASSERT_EQ(out, Dec4{0});  // setting `out` to 0 on failure is deprecated

  out = Dec4{1};
  ASSERT_TRUE(decimal64::fromString(".0", out));
  ASSERT_EQ(out, Dec4{0});

  out = Dec4{1};
  ASSERT_TRUE(decimal64::fromString(".0 ", out));
  ASSERT_EQ(out, Dec4{0});

  ASSERT_FALSE(decimal64::fromString("10a", out));

  ASSERT_FALSE(decimal64::fromString("10 a", out));
  ASSERT_TRUE(decimal64::fromString(" .01 ", out));
  ASSERT_EQ(out, Dec4{0.01});

  ASSERT_EQ(decimal64::fromString<Dec4>("    1"), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("1    "), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("  1  "), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("1"), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("1."), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("+1."), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("-1."), Dec4{-1});
  ASSERT_EQ(decimal64::fromString<Dec4>(".1"), Dec4{0.1});
  ASSERT_EQ(decimal64::fromString<Dec4>("+1."), Dec4{1});
  ASSERT_EQ(decimal64::fromString<Dec4>("0.0000501"), Dec4{0.0001});
  ASSERT_EQ(decimal64::fromString<Dec4>("12345.7890000"), Dec4{12345.789});
  ASSERT_EQ(decimal64::fromString<Dec4>("1000000000"), Dec4{1000000000});
  ASSERT_EQ(decimal64::fromString<Dec4>("-1000000000"), Dec4{-1000000000});

  ASSERT_FALSE(decimal64::fromString("", out));
  ASSERT_FALSE(decimal64::fromString("-", out));
  ASSERT_FALSE(decimal64::fromString("+", out));
  ASSERT_FALSE(decimal64::fromString(".", out));
  ASSERT_FALSE(decimal64::fromString("i  1", out));
  ASSERT_FALSE(decimal64::fromString("1  i", out));
  ASSERT_FALSE(decimal64::fromString("1a", out));
  ASSERT_FALSE(decimal64::fromString("10a", out));
  ASSERT_FALSE(decimal64::fromString("1,000.000", out));

  ASSERT_EQ(
      decimal64::fromString<decimal64::decimal<18>>("0.987654321123456789")
          .AsUnbiased(),
      987654321123456789LL);
  ASSERT_EQ(
      decimal64::fromString<decimal64::decimal<18>>("0.0000000000000000126")
          .AsUnbiased(),
      13);
  ASSERT_FALSE(decimal64::fromString("1234567898765432.1012", out));
  ASSERT_FALSE(decimal64::fromString("99999999999999999999999999", out));
  ASSERT_FALSE(decimal64::fromString("+99999999999999999999999999", out));
  ASSERT_FALSE(decimal64::fromString("-99999999999999999999999999", out));
}

TEST(Decimal64, FromStream) {
  Dec4 out;
  std::string remaining;

  out = Dec4{1};
  std::istringstream is1{"   \t  \r-10. \t  "};
  ASSERT_TRUE(decimal64::fromStream(is1, out));
  ASSERT_EQ(out, Dec4{-10});
  std::getline(is1, remaining);
  ASSERT_EQ(remaining, " \t  ");

  out = Dec4{1};
  std::istringstream is2{"12345678987654321 ab"};
  ASSERT_FALSE(decimal64::fromStream(is2, out));
  ASSERT_EQ(out, Dec4{0});  // setting `out` to 0 on failure is deprecated
  is2.clear();
  std::getline(is2, remaining);
  ASSERT_EQ(remaining, " ab");

  out = Dec4{1};
  std::istringstream is3{".-1"};
  ASSERT_FALSE(decimal64::fromStream(is3, out));
  ASSERT_EQ(out, Dec4{0});  // setting `out` to 0 on failure is deprecated
  is3.clear();
  std::getline(is3, remaining);
  ASSERT_EQ(remaining, "-1");
}

TEST(Decimal64, FromStreamDeprecated) {
  // Deprecated: extra characters adjacent to the decimal
  // These will return false and `fail()` the stream in a future revision

  Dec4 out;
  std::string remaining;

  std::istringstream is1{"123ab"};
  ASSERT_TRUE(decimal64::fromStream(is1, out));
  ASSERT_EQ(out, Dec4{123});
  std::getline(is1, remaining);
  ASSERT_EQ(remaining, "ab");

  std::istringstream is2{"1, 2, 3"};
  ASSERT_TRUE(decimal64::fromStream(is2, out));
  ASSERT_EQ(out, Dec4{1});
  std::getline(is2, remaining);
  ASSERT_EQ(remaining, ", 2, 3");

  std::istringstream is3{"-1.2.3"};
  ASSERT_TRUE(decimal64::fromStream(is3, out));
  ASSERT_EQ(out, Dec4{-1.2});
  std::getline(is3, remaining);
  ASSERT_EQ(remaining, ".3");

  std::istringstream is4{"-0.-1"};
  ASSERT_TRUE(decimal64::fromStream(is4, out));
  ASSERT_EQ(out, Dec4{0});
  std::getline(is4, remaining);
  ASSERT_EQ(remaining, "-1");
}
