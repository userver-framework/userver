#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <boost/math/special_functions.hpp>

#include <storages/postgres/io/boost_multiprecision.hpp>
#include <storages/postgres/io/user_types.hpp>
#include <storages/postgres/tests/test_buffers.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

using namespace io::traits;

static_assert(kHasTextFormatter<pg::Numeric>, "");
static_assert(kHasTextParser<pg::Numeric>, "");
static_assert(kHasBinaryParser<pg::Numeric>, "");
static_assert(kIsMappedToPg<pg::Numeric>, "");
static_assert(kTypeBufferCategory<pg::Numeric> ==
                  io::BufferCategory::kPlainBuffer,
              "");

}  // namespace static_test

namespace {

const pg::UserTypes types;

TEST(PostgreIO, Numeric) {
  {
    pg::Numeric src{"3.14"};
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(
        io::WriteBuffer<io::DataFormat::kTextDataFormat>(types, buffer, src));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
    pg::Numeric tgt{0};
    EXPECT_NO_THROW(io::ReadBuffer<io::DataFormat::kTextDataFormat>(fb, tgt));
    EXPECT_EQ(src, tgt);
  }
  {
    pg::Numeric src = boost::math::sin_pi(pg::Numeric{1});
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(
        io::WriteBuffer<io::DataFormat::kTextDataFormat>(types, buffer, src));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
    pg::Numeric tgt{0};
    EXPECT_NO_THROW(io::ReadBuffer<io::DataFormat::kTextDataFormat>(fb, tgt));
    EXPECT_EQ(0, src.compare(tgt))
        << "Number written to the buffer "
        << std::setprecision(std::numeric_limits<pg::Numeric>::digits10) << src
        << " is expected to be equal to number read from buffer " << tgt;
  }
}

class PostgreNumericIO : public ::testing::TestWithParam<std::string> {};

TEST_P(PostgreNumericIO, ParseString) {
  auto str_rep = GetParam();
  auto str_buf = io::detail::StringToNumericBuffer(str_rep);
  EXPECT_FALSE(str_buf.empty());
  pg::Numeric num{str_rep.c_str()};
  auto fb =
      pg::test::MakeFieldBuffer(str_buf, io::DataFormat::kBinaryDataFormat);
  pg::Numeric tgt;
  EXPECT_NO_THROW(io::ReadBinary(fb, tgt));
  if (str_rep != "nan")
    EXPECT_EQ(num, tgt) << "Expected " << num << " parsed " << tgt;
}

INSTANTIATE_TEST_CASE_P(
    PostgreIO, PostgreNumericIO,
    ::testing::Values("0", "nan", ".0", ".1", "-.5", "10", "100", "1000",
                      "1000000", "100000.0000001", ".001", "0.0001",
                      "0.00000000001", "1.000000001", "00000", "000.000000",
                      "10000."
                      "00000000000000000000000000000000000000000000000000000000"
                      "000000000000000000000000",
                      "0."
                      "00000000100000000000000000000000000000000000000000000000"
                      "000000000000000000000000"),
    /**/);

POSTGRE_TEST_P(NumericRoundtrip) {
  ASSERT_TRUE(conn.get());
  pg::ResultSet res{nullptr};

  EXPECT_EQ(io::BufferCategory::kPlainBuffer,
            io::GetBufferCategory(io::PredefinedOids::kNumeric));

  std::vector<pg::Numeric> test_values{
      pg::Numeric{"0"},       pg::Numeric{"0.0"},
      pg::Numeric{"0.01"},    pg::Numeric{"0.000001"},
      pg::Numeric{"0.00001"}, pg::Numeric{"0.000000001"},
      pg::Numeric{"10000"},   pg::Numeric{"99999999"},
      pg::Numeric{"-100500"}, pg::Numeric{"3.14159265358979323846"}};

  for (auto n : test_values) {
    EXPECT_NO_THROW(res = conn->Execute("select $1", n));
    pg::Numeric v;
    EXPECT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
    EXPECT_NO_THROW(v = res[0][0].As<pg::Numeric>());
    EXPECT_EQ(n, v) << n << " is not equal to " << v;
  }
}

}  // namespace
