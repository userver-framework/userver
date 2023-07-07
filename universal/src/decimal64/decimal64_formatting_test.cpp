#include <userver/decimal64/decimal64.hpp>

#include <sstream>

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <userver/decimal64/format_options.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/fmt_compat.hpp>

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

TEST(Decimal64, ToStringFormatOptions) {
  // clang-format off
  Dec4 dec4{"1034.1234"};
  EXPECT_EQ(ToString(dec4, {"||", "**", "\1",   "-", {},  1,  true}), "1**0**3**4||1");
  EXPECT_EQ(ToString(dec4, {"|",  "*",  "\1\2", "-", {},  {}, true}), "1*03*4|1234");
  EXPECT_EQ(ToString(dec4, {"|",  "*",  "\4",   "-", {},  {}, true}), "1034|1234");
  EXPECT_EQ(ToString(dec4, {"|",  "*",  "",     "-", {},  {}, true}), "1034|1234");
  EXPECT_EQ(ToString(dec4, {"",   "",   "\1",   "-", {},  {}, true}), "10341234");

  dec4 = Dec4{"-1000.1234"};
  EXPECT_EQ(ToString(dec4, {",", " ",       "\3", "-", {},  {}, true  }),   "-1 000,1234");
  EXPECT_EQ(ToString(dec4, {",", " ",       "\3", "-", {},  0,  true  }),   "-1 000");
  EXPECT_EQ(ToString(dec4, {",", "\u00a0",  "\3", "-", {},  0,  true  }),   "-1\u00a0000");
  EXPECT_EQ(ToString(dec4, {",", " ",       "\3", "-", {},  2,  true  }),   "-1 000,12");
  EXPECT_EQ(ToString(dec4, {",", " ",       "\3", "-", {},  6,  true  }),   "-1 000,123400");
  EXPECT_EQ(ToString(dec4, {",", " ",       "\3", "-", {},  6,  false }),   "-1 000,1234");

  decimal64::FormatOptions format_dec3{",", " ", "\3", "<>", "<+>", 3, false};
  EXPECT_EQ(ToString(Dec4{"-1234.1200"},  format_dec3), "<>1 234,12");
  EXPECT_EQ(ToString(Dec4{"1234.1200"},   format_dec3), "<+>1 234,12");
  EXPECT_EQ(ToString(Dec4{"-1234.0000"},  format_dec3), "<>1 234");
  EXPECT_EQ(ToString(Dec4{"-1234.0001"},  format_dec3), "<>1 234");
  EXPECT_EQ(ToString(Dec4{"-0.001"},      format_dec3), "<>0,001");
  EXPECT_EQ(ToString(Dec4{"-0.0004"},     format_dec3), "<+>0");
  EXPECT_EQ(ToString(Dec4{"0"},           format_dec3), "<+>0");

  EXPECT_EQ(ToString(Dec4{"-0.001"},  {",", "\3", " ", "-", {}, 2, true }), "0,00");
  EXPECT_EQ(ToString(Dec4{"-0.001"},  {",", "\3", " ", "-", {}, 0, true }), "0");
  EXPECT_EQ(ToString(Dec4{"0"},       {",", "\3", " ", "-", {}, 6, false}), "0");
  EXPECT_EQ(ToString(Dec4{"0"},       {",", "\3", " ", "-", {}, 0, true }), "0");

  decimal64::Decimal<0> dec0{"1234"};
  EXPECT_EQ(ToString(dec0, {",", " ", "\3",         "-", {}, {},  true  }),   "1 234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\3",         "-", {}, {},  false }),   "1 234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\3",         "-", {}, 2,   true  }),   "1 234,00");
  EXPECT_EQ(ToString(dec0, {",", " ", "\3",         "-", {}, 2,   false }),   "1 234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\4",         "-", {}, {},  true  }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\4",         "-", {}, {},  false }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\4\1",       "-", {}, {},  true  }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\4\1",       "-", {}, {},  false }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", {'\0'},       "-", {}, {},  true  }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", {'\0'},       "-", {}, {},  false }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", {'\4', '\0'}, "-", {}, {},  true  }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", {'\4', '\0'}, "-", {}, {},  false }),   "1234");

  decimal64::Decimal<2> dec2{"1010"};
  EXPECT_EQ(ToString(dec2, {",", " ", "\3", "-", {}, {}, false}), "1 010");
  EXPECT_EQ(ToString(dec2, {",", " ", "\3", "-", {}, {}, true }), "1 010,00");

  EXPECT_EQ(ToString(dec0, {",", " ", {'\1', '\0', '\1'}, "-", {}, {}, true }),   "123 4");
  EXPECT_EQ(ToString(dec0, {",", " ", {'\1', '\0', '\1'}, "-", {}, {}, false}),   "123 4");
  EXPECT_EQ(ToString(dec0, {",", " ", "\xAA",             "-", {}, {}, true }),   "1234");
  EXPECT_EQ(ToString(dec0, {",", " ", "\xAA",             "-", {}, {}, false}),   "1234");

  EXPECT_EQ(ToString(Dec4{"-1234.0101"},  {}),  "-1234.0101");
  EXPECT_EQ(ToString(Dec4{"1234.0101"},   {}),  "1234.0101");
  // clang-format on
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

TEST(Decimal64, ToStringFixed) {
  EXPECT_EQ(decimal64::ToStringFixed<3>(Dec4{"1000"}), "1000.000");
  EXPECT_EQ(decimal64::ToStringFixed<2>(Dec4{"0"}), "0.00");
  EXPECT_EQ(decimal64::ToStringFixed<0>(Dec4{"1"}), "1");
  EXPECT_EQ(decimal64::ToStringFixed<3>(Dec4{"1.1"}), "1.100");
  EXPECT_EQ(decimal64::ToStringFixed<1>(Dec4{"1.01"}), "1.0");
  EXPECT_EQ(decimal64::ToStringFixed<2>(Dec4{"1.06"}), "1.06");
  EXPECT_EQ(decimal64::ToStringFixed<0>(Dec4{"1.0001"}), "1");
  EXPECT_EQ(decimal64::ToStringFixed<3>(Dec4{"-20"}), "-20.000");
  EXPECT_EQ(decimal64::ToStringFixed<2>(Dec4{"-0.1"}), "-0.10");
  EXPECT_EQ(decimal64::ToStringFixed<0>(Dec4{"-0.0001"}), "0");
  EXPECT_EQ(decimal64::ToStringFixed<0>(Dec4{"12.34"}), "12");
  EXPECT_EQ(decimal64::ToStringFixed<0>(Dec4{"-12.34"}), "-12");
  EXPECT_EQ(decimal64::ToStringFixed<2>(decimal64::Decimal<18>{"1"}), "1.00");
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
  EXPECT_EQ(fmt::format("{:.5}", Dec4{"12.34"}), "12.34000");
  EXPECT_EQ(fmt::format("{:.2}", Dec4{"12.34"}), "12.34");
  EXPECT_EQ(fmt::format("{:.1}", Dec4{"12.34"}), "12.3");
  EXPECT_THROW(
      static_cast<void>(fmt::format(fmt::runtime("{:.5f}"), Dec4{"12.34"})),
      fmt::format_error);
  EXPECT_THROW(static_cast<void>(fmt::format(fmt::runtime("{:s}"), Dec4{42})),
               fmt::format_error);
  EXPECT_THROW(static_cast<void>(fmt::format(fmt::runtime("{:s}"), Dec4{42})),
               fmt::format_error);
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
