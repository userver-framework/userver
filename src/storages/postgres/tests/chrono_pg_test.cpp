#include <gtest/gtest.h>

#include <storages/postgres/io/chrono.hpp>
#include <storages/postgres/test_buffers.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

static_assert(
    (io::traits::HasFormatter<std::chrono::system_clock::time_point,
                              io::DataFormat::kTextDataFormat>::value == true),
    "");
static_assert((io::traits::HasParser<std::chrono::system_clock::time_point,
                                     io::DataFormat::kTextDataFormat>::value ==
               true),
              "");
static_assert(
    (io::traits::HasFormatter<std::chrono::high_resolution_clock::time_point,
                              io::DataFormat::kTextDataFormat>::value == true),
    "");
static_assert(
    (io::traits::HasParser<std::chrono::high_resolution_clock::time_point,
                           io::DataFormat::kTextDataFormat>::value == true),
    "");

}  // namespace static_test

TEST(PostgreIO, Chrono) {
  {
    auto now = std::chrono::system_clock::now();
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(
        io::WriteBuffer<io::DataFormat::kTextDataFormat>(buffer, now));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
    std::chrono::system_clock::time_point tgt;
    EXPECT_NO_THROW(io::ReadBuffer<io::DataFormat::kTextDataFormat>(fb, tgt));
    EXPECT_EQ(now, tgt) << "Parse buffer "
                        << std::string{buffer.begin(), buffer.end()};
  }
  {
    auto now = std::chrono::high_resolution_clock::now();
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(
        io::WriteBuffer<io::DataFormat::kTextDataFormat>(buffer, now));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
    std::chrono::high_resolution_clock::time_point tgt;
    EXPECT_NO_THROW(io::ReadBuffer<io::DataFormat::kTextDataFormat>(fb, tgt));
    EXPECT_EQ(now, tgt) << "Parse buffer "
                        << std::string{buffer.begin(), buffer.end()};
  }
}
