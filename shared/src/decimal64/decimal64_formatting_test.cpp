#include <userver/decimal64/decimal64.hpp>

#include <sstream>

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

using Dec4 = decimal64::Decimal<4>;

TEST(Decimal64, ToString) {
  EXPECT_EQ(decimal64::ToString(Dec4{"1000"}), "1000");
  EXPECT_EQ(decimal64::ToString(Dec4{"0"}), "0");
  EXPECT_EQ(decimal64::ToString(Dec4{"1"}), "1");
  EXPECT_EQ(decimal64::ToString(Dec4{"1.1"}), "1.1");
  EXPECT_EQ(decimal64::ToString(Dec4{"1.01"}), "1.01");
  EXPECT_EQ(decimal64::ToString(Dec4{"1.001"}), "1.001");
  EXPECT_EQ(decimal64::ToString(Dec4{"1.0001"}), "1.0001");
  EXPECT_EQ(decimal64::ToString(Dec4{"-20"}), "-20");
  EXPECT_EQ(decimal64::ToString(Dec4{"-0.1"}), "-0.1");
  EXPECT_EQ(decimal64::ToString(Dec4{"-0.0001"}), "-0.0001");
  EXPECT_EQ(decimal64::ToString(Dec4{"12.34"}), "12.34");
  EXPECT_EQ(decimal64::ToString(Dec4{"-12.34"}), "-12.34");
  EXPECT_EQ(decimal64::ToString(Dec4{"-0.34"}), "-0.34");
  EXPECT_EQ(decimal64::ToString(Dec4{"-12.0"}), "-12");
  EXPECT_EQ(decimal64::ToString(decimal64::Decimal<18>{"1"}), "1");
  EXPECT_EQ(decimal64::ToString(decimal64::Decimal<5>{"1"}), "1");
  EXPECT_EQ(decimal64::ToString(decimal64::Decimal<0>{"1"}), "1");
}

TEST(Decimal64, ToStringTrailingZeros) {
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1000"}), "1000.0000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"0"}), "0.0000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1"}), "1.0000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.1"}), "1.1000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.01"}), "1.0100");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.001"}), "1.0010");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.0001"}), "1.0001");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"-20"}), "-20.0000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"-0.1"}), "-0.1000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(Dec4{"-0.0001"}), "-0.0001");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(decimal64::Decimal<18>{"1"}),
            "1.000000000000000000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(decimal64::Decimal<5>{"1"}),
            "1.00000");
  EXPECT_EQ(decimal64::ToStringTrailingZeros(decimal64::Decimal<0>{"1"}), "1");
}

TEST(Decimal64, ZerosFullyTrimmed) {
  EXPECT_EQ(ToString(decimal64::Decimal<0>{"1"}), "1");
  EXPECT_EQ(ToString(decimal64::Decimal<1>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<2>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<3>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<4>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<5>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<6>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<7>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<8>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<9>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<10>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<11>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<12>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<13>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<14>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<15>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<16>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<17>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<18>{"0.1"}), "0.1");
}

TEST(Decimal64, Fmt) {
  EXPECT_EQ(fmt::to_string(Dec4{"12.34"}), "12.34");
  EXPECT_EQ(fmt::format("{}", Dec4{"12.34"}), "12.34");
  EXPECT_EQ(fmt::format("{:f}", Dec4{"12.34"}), "12.3400");
  EXPECT_THROW(fmt::format("{:s}", Dec4{42}), fmt::format_error);
}

TEST(Decimal64, SerializeCycle) {
  const Dec4 original{42};
  const auto json_object = formats::json::ValueBuilder(original).ExtractValue();
  EXPECT_EQ(json_object.As<Dec4>(), original);
}

TEST(Decimal64, ToStream) {
  std::ostringstream os;
  os << Dec4{"12.3"};
  EXPECT_EQ(os.str(), "12.3");
}

USERVER_NAMESPACE_END
