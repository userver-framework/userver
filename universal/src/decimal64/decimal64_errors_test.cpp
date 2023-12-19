#include <userver/decimal64/decimal64.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

using Dec4 = decimal64::Decimal<4>;

namespace {

std::string ParsingErrorMessage(std::string_view decimal_string) {
  try {
    [[maybe_unused]] const Dec4 result{decimal_string};
    return "success?!";
  } catch (const decimal64::ParseError& ex) {
    return ex.what();
  }
}

std::string PermissiveParsingErrorMessage(std::string_view decimal_string) {
  try {
    [[maybe_unused]] const auto result =
        Dec4::FromStringPermissive(decimal_string);
    return "success?!";
  } catch (const decimal64::ParseError& ex) {
    return ex.what();
  }
}

}  // namespace

TEST(Decimal64, WrongCharError) {
  EXPECT_EQ(ParsingErrorMessage("abc"),
            "While parsing Decimal at '<string>' from 'abc', error at char #1: "
            "wrong character");
}

TEST(Decimal64, NoDigitsError) {
  EXPECT_EQ(PermissiveParsingErrorMessage("-."),
            "While parsing Decimal at '<string>' from '-.', error at char #2: "
            "the input contains no digits");
}

TEST(Decimal64, OverflowError) {
  EXPECT_EQ(ParsingErrorMessage("1234567898765432"),
            "While parsing Decimal at '<string>' from '1234567898765432', "
            "error at char #1: overflow");
}

TEST(Decimal64, SpaceError) {
  EXPECT_EQ(ParsingErrorMessage("42 "),
            "While parsing Decimal at '<string>' from '42 ', error at char #3: "
            "extra spaces are disallowed");
}

TEST(Decimal64, TrailingJunkError) {
  EXPECT_EQ(ParsingErrorMessage("42x"),
            "While parsing Decimal at '<string>' from '42x', error at char #3: "
            "extra characters after the number are disallowed");
}

TEST(Decimal64, BoundaryDotError) {
  EXPECT_EQ(ParsingErrorMessage(".42"),
            "While parsing Decimal at '<string>' from '.42', error at char #1: "
            "the input includes leading or trailing dot (like '42.'), while "
            "such notation is disallowed");
}

TEST(Decimal64, RoundingError) {
  // 'at char #1' is not perfect, but will pass
  EXPECT_EQ(ParsingErrorMessage("0.00005"),
            "While parsing Decimal at '<string>' from '0.00005', error at char "
            "#1: the input contains more fractional digits than in target "
            "precision, while implicit rounding is disallowed");
}

USERVER_NAMESPACE_END
