#include <decimal64/decimal64.hpp>

#include <sstream>

#include <gtest/gtest.h>

using Dec4 = decimal64::Decimal<4>;

TEST(Decimal64, ConstructFromString) {
  EXPECT_EQ(Dec4{"10"}, Dec4{10});
  EXPECT_EQ(Dec4{"10."}, Dec4{10});
  EXPECT_EQ(Dec4{"+10"}, Dec4{10});
  EXPECT_EQ(Dec4{"-10"}, Dec4{-10});

  EXPECT_EQ(Dec4{"-0"}, Dec4{0});
  EXPECT_EQ(Dec4{"+.0"}, Dec4{0});
  EXPECT_EQ(Dec4{"-.0"}, Dec4{0});
  EXPECT_EQ(Dec4{"010"}, Dec4{10});
  EXPECT_EQ(Dec4{"000005"}, Dec4{5});

  EXPECT_EQ(Dec4{"000000000000000000000000000000012.0"}.AsUnbiased(), 12'0000);
  EXPECT_EQ(Dec4{"000000000000000000000000000000000.12"}.AsUnbiased(), 1200);
  EXPECT_EQ(Dec4{"12345678987654.3210"}.AsUnbiased(), 12345678987654'3210LL);

  EXPECT_EQ(decimal64::Decimal<18>{"0.987654321123456789"}.AsUnbiased(),
            987654321123456789LL);
  EXPECT_EQ(decimal64::Decimal<18>{"0.0000000000000000126"}.AsUnbiased(), 13);
  EXPECT_EQ(decimal64::Decimal<18>{"0.0000000000000000125"}.AsUnbiased(), 13);
  EXPECT_EQ(decimal64::Decimal<18>{"0.0000000000000000124"}.AsUnbiased(), 12);

  EXPECT_THROW(Dec4{".+0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{".-0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"#"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"1234567898765432.1012"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"99999999999999999999999999"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+99999999999999999999999999"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-99999999999999999999999999"}, decimal64::ParseError);
}

TEST(Decimal64, ConstructFromStringDeprecated) {
  // Deprecated: extra characters adjacent to the decimal
  // These will throw in a future revision
  EXPECT_EQ(Dec4{"123ab"}, Dec4{123});
  EXPECT_EQ(Dec4{"1#.1234"}, Dec4{1});
  EXPECT_EQ(Dec4{"12#.1234"}, Dec4{12});
  EXPECT_EQ(Dec4{"10.#123"}, Dec4{10});
  EXPECT_EQ(Dec4{"10.123#"}.AsUnbiased(), 10'1230);
  EXPECT_EQ(Dec4{"0x10"}, Dec4{0});
  EXPECT_EQ(Dec4{"+0x10"}, Dec4{0});
  EXPECT_EQ(Dec4{"-0x10"}, Dec4{0});
  EXPECT_EQ(Dec4{"0x1a"}, Dec4{0});
  EXPECT_EQ(Dec4{"+0x1a"}, Dec4{0});
  EXPECT_EQ(Dec4{"-0x1a"}, Dec4{0});

  // Deprecated: extra tokens after the decimal
  // Use stream operations instead
  // These will throw in a future revision
  EXPECT_EQ(Dec4{"   \t  \r-10. \n  ab"}, Dec4{-10});
  EXPECT_EQ(Dec4{"1 0"}, Dec4{1});
  EXPECT_EQ(Dec4{"+1 0"}, Dec4{1});
  EXPECT_EQ(Dec4{"-1 0"}, Dec4{-1});
  EXPECT_EQ(Dec4{"1. 0"}, Dec4{1});
  EXPECT_EQ(Dec4{"+1. 0"}, Dec4{1});
  EXPECT_EQ(Dec4{"-1. 0"}, Dec4{-1});
  EXPECT_EQ(Dec4{"1 .0"}, Dec4{1});
  EXPECT_EQ(Dec4{"+1 .0"}, Dec4{1});
  EXPECT_EQ(Dec4{"-1 .0"}, Dec4{-1});
}

TEST(Decimal64, FromStringPermissive) {
  EXPECT_EQ(Dec4::FromStringPermissive("1234.5678"), Dec4{"1234.5678"});
  EXPECT_EQ(Dec4::FromStringPermissive(".0"), Dec4{0});
  EXPECT_EQ(Dec4::FromStringPermissive(" .01 "), Dec4{"0.01"});
  EXPECT_EQ(Dec4::FromStringPermissive("    1"), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("1    "), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("  1  "), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("1"), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("1."), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("+1."), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("-1."), Dec4{-1});
  EXPECT_EQ(Dec4::FromStringPermissive(".1"), Dec4{"0.1"});
  EXPECT_EQ(Dec4::FromStringPermissive("+1."), Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("0.0000501"), Dec4{"0.0001"});
  EXPECT_EQ(Dec4::FromStringPermissive("12345.7890000"), Dec4{"12345.789"});
  EXPECT_EQ(Dec4::FromStringPermissive("1000000000"), Dec4{1000000000});
  EXPECT_EQ(Dec4::FromStringPermissive("-1000000000"), Dec4{-1000000000});

  EXPECT_THROW(Dec4::FromStringPermissive("1 0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+1 0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("-1 0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("1. 0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+1. 0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("-1. 0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("1 .0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+1 .0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("-1 .0"), decimal64::ParseError);
  ASSERT_THROW(Dec4::FromStringPermissive("0x10"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+0x10"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("-0x10"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+0x1a"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("10a"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("10 a"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("1  i"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("1a"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("10a"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("1,000.000"), decimal64::ParseError);

  EXPECT_EQ(decimal64::Decimal<18>::FromStringPermissive("0.987654321123456789")
                .AsUnbiased(),
            987654321123456789LL);
  EXPECT_EQ(
      decimal64::Decimal<18>::FromStringPermissive("0.0000000000000000126")
          .AsUnbiased(),
      13);

  EXPECT_THROW(Dec4::FromStringPermissive(".+0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive(".-0"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive(""), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("-"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+"), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("."), decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("i  1"), decimal64::ParseError);

  EXPECT_THROW(Dec4::FromStringPermissive("1234567898765432.1012"),
               decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("99999999999999999999999999"),
               decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("+99999999999999999999999999"),
               decimal64::ParseError);
  EXPECT_THROW(Dec4::FromStringPermissive("-99999999999999999999999999"),
               decimal64::ParseError);
}

TEST(Decimal64, FromStringStrict) {
  EXPECT_EQ(ToString(decimal64::impl::FromString<Dec4>("1234.5678")),
            "1234.5678");
  EXPECT_EQ(ToString(decimal64::impl::FromString<Dec4>("1234.567")),
            "1234.567");
  EXPECT_EQ(ToString(decimal64::impl::FromString<Dec4>("1234")), "1234");
  EXPECT_EQ(ToString(decimal64::impl::FromString<Dec4>("0.123")), "0.123");

  EXPECT_THROW(decimal64::impl::FromString<Dec4>(""), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("."), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("#"), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>(" 1"), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1 "), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1.."), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1a"), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1000000000000000"),
               decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1000000000000000000000"),
               decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1.23456"),
               decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("1.23450"),
               decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>("42."), decimal64::ParseError);
  EXPECT_THROW(decimal64::impl::FromString<Dec4>(".42"), decimal64::ParseError);
}

namespace {

constexpr Dec4 kUnchangedOnError{42};

void TestFromStream(const std::string& input, bool expected_success,
                    Dec4 expected_result, std::string_view expected_remaining) {
  Dec4 result = kUnchangedOnError;
  std::string remaining;
  std::istringstream is{input};
  EXPECT_EQ(static_cast<bool>(is >> result), expected_success)
      << "input: \"" << input << "\"";
  is.clear();
  std::getline(is, remaining);
  EXPECT_EQ(result, expected_result) << "input: \"" << input << "\"";
  EXPECT_EQ(remaining, expected_remaining) << "input: \"" << input << "\"";
}

}  // namespace

TEST(Decimal64, FromStream) {
  TestFromStream("123", true, Dec4{123}, "");
  TestFromStream("   \t  \r-10.0 \t  ", true, Dec4{-10}, " \t  ");
  TestFromStream("123ab", true, Dec4{123}, "ab");
  TestFromStream("1, 2, 3", true, Dec4{1}, ", 2, 3");
  TestFromStream("-1.2.3", true, Dec4{"-1.2"}, ".3");
  TestFromStream("-0.-1", false, kUnchangedOnError, "-1");
  TestFromStream("12345678987654321 ab", false, kUnchangedOnError, " ab");
  TestFromStream(".-1", false, kUnchangedOnError, "-1");
}

TEST(Decimal64, FromStreamFlags) {
  Dec4 result = kUnchangedOnError;
  std::istringstream is{"123"};
  is >> result;

  EXPECT_EQ(result, Dec4{123});
  EXPECT_FALSE(is.good());
  EXPECT_FALSE(is.fail());
  EXPECT_FALSE(is.bad());
  EXPECT_TRUE(is.eof());
  EXPECT_TRUE(static_cast<bool>(is));
  EXPECT_FALSE(!is);
}
