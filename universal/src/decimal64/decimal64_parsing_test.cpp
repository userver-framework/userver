#include <userver/decimal64/decimal64.hpp>

#include <ios>
#include <sstream>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

using Dec4 = decimal64::Decimal<4>;

// NOLINTNEXTLINE(readability-function-size)
TEST(Decimal64, ConstructFromString) {
  EXPECT_EQ(Dec4{"10"}, Dec4{10});
  EXPECT_EQ(Dec4{"+10"}, Dec4{10});
  EXPECT_EQ(Dec4{"-10"}, Dec4{-10});
  EXPECT_EQ(Dec4{"12345678987654.3210"}.AsUnbiased(), 12345678987654'3210LL);
  EXPECT_EQ(decimal64::Decimal<18>{"0.987654321123456789"}.AsUnbiased(),
            987654321123456789LL);

  // Leading zeros
  EXPECT_EQ(Dec4{"-0"}, Dec4{0});
  EXPECT_EQ(Dec4{"010"}, Dec4{10});
  EXPECT_EQ(Dec4{"000005"}, Dec4{5});
  EXPECT_EQ(Dec4{"000000000000000000000000000000012.0"}.AsUnbiased(), 12'0000);
  EXPECT_EQ(Dec4{"000000000000000000000000000000000.12"}.AsUnbiased(), 1200);

  // Garbage
  EXPECT_THROW(Dec4{".+0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{".-0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"#"}, decimal64::ParseError);

  // Boundary dot
  EXPECT_THROW(Dec4{"10."}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+.0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-.0"}, decimal64::ParseError);

  // Overflow
  EXPECT_THROW(Dec4{"1234567898765432.1012"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-1234567898765432.1012"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"99999999999999999999999999"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+99999999999999999999999999"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-99999999999999999999999999"}, decimal64::ParseError);

  // Precision loss
  EXPECT_THROW(Dec4{"123.45678"}, decimal64::ParseError);
  EXPECT_THROW(decimal64::Decimal<18>{"0.0000000000000000126"},
               decimal64::ParseError);
  EXPECT_THROW(decimal64::Decimal<18>{"0.0000000000000000125"},
               decimal64::ParseError);
  EXPECT_THROW(decimal64::Decimal<18>{"0.0000000000000000124"},
               decimal64::ParseError);

  // Extra characters adjacent to the decimal
  EXPECT_THROW(Dec4{"123ab"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"1#.1234"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"12#.1234"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"10.#123"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"10.123#"}.AsUnbiased(), decimal64::ParseError);
  EXPECT_THROW(Dec4{"0x10"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+0x10"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-0x10"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"0x1a"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+0x1a"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-0x1a"}, decimal64::ParseError);

  // Extra tokens after the decimal
  EXPECT_THROW(Dec4{"   \t  \r-10. \n  ab"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"1 0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+1 0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-1 0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"1. 0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+1. 0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-1. 0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"1 .0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"+1 .0"}, decimal64::ParseError);
  EXPECT_THROW(Dec4{"-1 .0"}, decimal64::ParseError);
}

// NOLINTNEXTLINE(readability-function-size)
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

namespace {

constexpr Dec4 kUnchangedOnError{42};

void TestFromStream(const std::string& input, bool expected_success,
                    Dec4 expected_result, std::string_view expected_remaining) {
  Dec4 result = kUnchangedOnError;
  std::istringstream is{input};
  EXPECT_EQ(static_cast<bool>(is >> result), expected_success)
      << "input: \"" << input << "\"";
  is.clear();
  const std::string remaining(std::istreambuf_iterator<char>(is), {});
  EXPECT_EQ(result, expected_result) << "input: \"" << input << "\"";
  EXPECT_EQ(remaining, expected_remaining) << "input: \"" << input << "\"";
}

}  // namespace

TEST(Decimal64, FromStream) {
  TestFromStream("123", true, Dec4{123}, "");
  TestFromStream("   \n  -10.0 \t  ", true, Dec4{-10}, " \t  ");
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

TEST(Decimal64, FromStreamNoSkipWs) {
  Dec4 result = kUnchangedOnError;
  std::istringstream is{" 123"};
  is >> std::noskipws >> result;

  EXPECT_EQ(result, kUnchangedOnError);
  EXPECT_TRUE(is.fail());
}

TEST(Decimal64, ParseValid) {
  const auto json_object =
      formats::json::FromString(R"json({"data" : "123"})json");
  const auto json_data = json_object["data"];
  EXPECT_EQ(json_data.As<Dec4>(), Dec4{123});
}

TEST(Decimal64, ParseInvalid) {
  const auto json_object =
      formats::json::FromString(R"json({"data" : "#"})json");
  const auto json_data = json_object["data"];
  EXPECT_THROW(json_data.As<Dec4>(), decimal64::ParseError);
}

TEST(Decimal64, RoundPolicies) {
  using Def = decimal64::Decimal<4, decimal64::DefRoundPolicy>;
  EXPECT_EQ(Def::FromStringPermissive("0.00004999999999999"), Def{0});
  EXPECT_EQ(Def::FromStringPermissive("0.00005"), Def{"0.0001"});

  using HalfDown = decimal64::Decimal<4, decimal64::HalfDownRoundPolicy>;
  EXPECT_EQ(HalfDown::FromStringPermissive("0.00005"), HalfDown{0});
  EXPECT_EQ(HalfDown::FromStringPermissive("0.00005000000000001"),
            HalfDown{"0.0001"});
  EXPECT_EQ(HalfDown::FromStringPermissive("-0.00005"), HalfDown{0});
  EXPECT_EQ(HalfDown::FromStringPermissive("-0.00005000000000001"),
            HalfDown{"-0.0001"});

  using HalfUp = decimal64::Decimal<4, decimal64::HalfUpRoundPolicy>;
  EXPECT_EQ(HalfUp::FromStringPermissive("0.00004999999999999"), HalfUp{0});
  EXPECT_EQ(HalfUp::FromStringPermissive("0.00005"), HalfUp{"0.0001"});
  EXPECT_EQ(HalfUp::FromStringPermissive("-0.00004999999999999"), HalfUp{0});
  EXPECT_EQ(HalfUp::FromStringPermissive("-0.00005"), HalfUp{"-0.0001"});

  using HalfEven = decimal64::Decimal<4, decimal64::HalfEvenRoundPolicy>;
  EXPECT_EQ(HalfEven::FromStringPermissive("0.00005"), HalfEven{0});
  EXPECT_EQ(HalfEven::FromStringPermissive("0.00015"), HalfEven{"0.0002"});
  EXPECT_EQ(HalfEven::FromStringPermissive("-0.00005"), HalfEven{0});
  EXPECT_EQ(HalfEven::FromStringPermissive("-0.00015"), HalfEven{"-0.0002"});

  using Ceiling = decimal64::Decimal<4, decimal64::CeilingRoundPolicy>;
  EXPECT_EQ(Ceiling::FromStringPermissive("0.00000000000000001"),
            Ceiling{"0.0001"});
  EXPECT_EQ(Ceiling::FromStringPermissive("-0.000099999999999999"), Ceiling{0});

  using Floor = decimal64::Decimal<4, decimal64::FloorRoundPolicy>;
  EXPECT_EQ(Floor::FromStringPermissive("0.000099999999999999"), Floor{0});
  EXPECT_EQ(Floor::FromStringPermissive("-0.00000000000000001"),
            Floor{"-0.0001"});

  using Ceiling = decimal64::Decimal<4, decimal64::CeilingRoundPolicy>;
  EXPECT_EQ(Ceiling::FromStringPermissive("0.00000000000000001"),
            Ceiling{"0.0001"});
  EXPECT_EQ(Ceiling::FromStringPermissive("-0.000099999999999999"), Ceiling{0});

  using Up = decimal64::Decimal<4, decimal64::RoundUpRoundPolicy>;
  EXPECT_EQ(Up::FromStringPermissive("0.00000000000000001"), Up{"0.0001"});
  EXPECT_EQ(Up::FromStringPermissive("-0.00000000000000001"), Up{"-0.0001"});

  using Down = decimal64::Decimal<4, decimal64::RoundDownRoundPolicy>;
  EXPECT_EQ(Down::FromStringPermissive("0.000099999999999999"), Down{0});
  EXPECT_EQ(Down::FromStringPermissive("-0.000099999999999999"), Down{0});
}

USERVER_NAMESPACE_END
