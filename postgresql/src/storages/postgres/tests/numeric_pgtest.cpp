#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <boost/math/special_functions.hpp>

#include <storages/postgres/tests/test_buffers.hpp>
#include <userver/storages/postgres/io/boost_multiprecision.hpp>
#include <userver/storages/postgres/io/decimal64.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

namespace tt = io::traits;

using Numeric = pg::MultiPrecision<50>;

static_assert(pg::detail::kIsInBoostNamespace<Numeric>);
static_assert(sizeof(Numeric));
static_assert(sizeof(io::BufferParser<Numeric>));
static_assert(sizeof(io::BufferFormatter<Numeric>));

static_assert(tt::kHasFormatter<Numeric>);
static_assert(tt::kHasParser<Numeric>);
static_assert(tt::kIsMappedToPg<Numeric>);
static_assert(tt::kTypeBufferCategory<Numeric> ==
              io::BufferCategory::kPlainBuffer);

}  // namespace static_test

namespace {

using Numeric = pg::MultiPrecision<50>;
const pg::UserTypes types;

TEST(PostgreIO, Numeric) {
  {
    Numeric src{"3.14"};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    Numeric tgt{0};
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(src, tgt);
  }
  {
    Numeric src{0};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    Numeric tgt{0};
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(0, src.compare(tgt))
        << "Number written to the buffer "
        << std::setprecision(std::numeric_limits<Numeric>::digits10) << src
        << " is expected to be equal to number read from buffer " << tgt;
  }
}

class PostgreNumericIO : public ::testing::TestWithParam<std::string> {};

TEST_P(PostgreNumericIO, ParseString) {
  auto str_rep = GetParam();
  auto str_buf = io::detail::StringToNumericBuffer(str_rep);
  EXPECT_FALSE(str_buf.empty());
  Numeric num{str_rep.c_str()};
  auto fb = pg::test::MakeFieldBuffer(str_buf);
  Numeric tgt;
  UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
  if (str_rep != "nan") {
    EXPECT_EQ(num, tgt) << "Expected " << num << " parsed " << tgt;
  }
}

INSTANTIATE_TEST_SUITE_P(
    PostgreIO, PostgreNumericIO,
    ::testing::Values("0", "nan", ".0", ".1", "-.5", "10", "100", "1000",
                      "1000000", "100000.0000001", ".001", "0.0001",
                      "0.00000000001", "1.000000001", "00000", "000.000000",
                      "10000."
                      "00000000000000000000000000000000000000000000000000000000"
                      "000000000000000000000000",
                      "0."
                      "00000000100000000000000000000000000000000000000000000000"
                      "000000000000000000000000"));

UTEST_P(PostgreConnection, NumericRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  EXPECT_EQ(io::BufferCategory::kPlainBuffer,
            io::GetBufferCategory(io::PredefinedOids::kNumeric));

  std::vector<Numeric> test_values{
      Numeric{"0"},       Numeric{"0.0"},
      Numeric{"0.01"},    Numeric{"0.000001"},
      Numeric{"0.00001"}, Numeric{"0.000000001"},
      Numeric{"10000"},   Numeric{"99999999"},
      Numeric{"-100500"}, Numeric{"3.14159265358979323846"}};

  for (auto n : test_values) {
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", n));
    Numeric v;
    UEXPECT_NO_THROW(v = res[0][0].As<Numeric>());
    EXPECT_EQ(n, v) << n << " is not equal to " << v;
  }
}

using DecIOTestData =
    std::pair<std::string, pg::io::detail::IntegralRepresentation>;

class PostgreDecimalIO : public ::testing::TestWithParam<DecIOTestData> {};

TEST_P(PostgreDecimalIO, BufferIO) {
  auto params = GetParam();
  auto expected = params.second;
  auto expected_str = params.first;

  auto buffer_str = pg::io::detail::StringToNumericBuffer(params.first);
  // pg::test::Buffer buffer{buffer_str.begin(), buffer_str.end()};
  auto fb = pg::test::MakeFieldBuffer(buffer_str);
  auto parsed = pg::io::detail::NumericBufferToInt64(fb);

  EXPECT_EQ(expected.value, parsed.value);
  EXPECT_EQ(expected.fractional_digit_count, parsed.fractional_digit_count);

  auto buffer_str2 = pg::io::detail::Int64ToNumericBuffer(parsed);
  EXPECT_EQ(buffer_str, buffer_str2)
      << "Formatted binary postgres buffers are equal";

  fb = pg::test::MakeFieldBuffer(buffer_str2);
  auto parsed_str = pg::io::detail::NumericBufferToString(fb);
  EXPECT_EQ(expected_str, parsed_str)
      << "The number string parsed out of the binary buffer is equal to the "
         "original";
}

std::string TestDescription(
    const ::testing::TestParamInfo<DecIOTestData>& info) {
  auto name = info.param.first;
  auto dot = name.find_first_of(".-");
  while (dot != std::string::npos) {
    name.replace(dot, 1, "_");
    dot = name.find_first_of(".-");
  }
  return name;
}

// ATTN: Don't use zero fractional part in this test, or reverse buffer
// conversion test will fail
INSTANTIATE_TEST_SUITE_P(
    PostgreIO, PostgreDecimalIO,
    ::testing::Values(
        DecIOTestData{"0", {0, 0}}, DecIOTestData{"1", {1, 0}},
        DecIOTestData{"-1", {-1, 0}}, DecIOTestData{"-1.01", {-101, 2}},
        DecIOTestData{"1000", {1000, 0}}, DecIOTestData{"9999", {9999, 0}},
        DecIOTestData{"10000", {10000, 0}}, DecIOTestData{"0.1", {1, 1}},
        DecIOTestData{"0.001", {1, 3}}, DecIOTestData{"0.0001", {1, 4}},
        DecIOTestData{"10000000", {10000000, 0}},
        DecIOTestData{"99999999", {99999999, 0}},
        DecIOTestData{"0.00001", {1, 5}},
        DecIOTestData{"10000.00001", {1000000001, 5}}),
    TestDescription);

UTEST_P(PostgreConnection, DecimalRoundtrip) {
  using Decimal = decimal64::Decimal<10>;

  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  EXPECT_EQ(io::BufferCategory::kPlainBuffer,
            io::GetBufferCategory(io::PredefinedOids::kNumeric));

  std::vector<Decimal> test_values{
      Decimal{"0"},           Decimal{"0.0"},     Decimal{"-1.0"},
      Decimal{"-0.01"},       Decimal{"0.01"},    Decimal{"0.001"},
      Decimal{"0.0001"},      Decimal{"0.00001"}, Decimal{"0.000001"},
      Decimal{"0.000000001"}, Decimal{"10000"},   Decimal{"1000"},
      Decimal{"4242"},        Decimal{"9999"},    Decimal{"10000000"},
      Decimal{"99999999"},    Decimal{"9999999"}, Decimal{"-100500"},
      Decimal{"3.1415926535"}};

  for (auto n : test_values) {
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", n));
    Decimal v;
    UEXPECT_NO_THROW(v = res[0][0].As<Decimal>());
    EXPECT_EQ(n, v) << n << " is not equal to " << v;
  }
}

UTEST_P(PostgreConnection, DecimalStored) {
  using Decimal = decimal64::Decimal<5>;

  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  Decimal expected{"2.71828"};
  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select $1", pg::ParameterStore{}.PushBack(expected)));
  Decimal decimal;
  UEXPECT_NO_THROW(res[0][0].To(decimal));
  EXPECT_EQ(decimal, expected);
}

}  // namespace

USERVER_NAMESPACE_END
