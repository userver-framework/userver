#include <gtest/gtest.h>

#include <boost/math/special_functions.hpp>

#include <storages/postgres/io/boost_multiprecision.hpp>
#include <storages/postgres/io/user_types.hpp>
#include <storages/postgres/test_buffers.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

using namespace io::traits;

static_assert(kHasTextFormatter<pg::Numeric>, "");
static_assert(kHasTextParser<pg::Numeric>, "");

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
        << "Number writter to the buffer "
        << std::setprecision(std::numeric_limits<pg::Numeric>::digits10) << src
        << " is expected to be equal to number read from buffer " << tgt;
  }
}

}  // namespace
