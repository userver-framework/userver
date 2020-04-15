#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

using namespace io::traits;

static_assert(kHasFormatter<bool>, "bool has a binary formatter");
static_assert(kHasParser<bool>, "bool has a binary parser");

static_assert(kHasFormatter<pg::Smallint>, "Smallint has a binary formatter");
static_assert(kHasParser<pg::Smallint>, "Smallint has a binary parser");

static_assert(kHasFormatter<pg::Integer>, "Integer has a binary formatter");
static_assert(kHasParser<pg::Integer>, "Integer has a binary parser");

static_assert(kHasFormatter<pg::Bigint>, "Bigint has a binary formatter");
static_assert(kHasParser<pg::Bigint>, "Bigint has a binary parser");

const pg::UserTypes types;

}  // namespace

TEST(PostgreIO, IntegralParserRegistry) {
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt2));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt4));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt8));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kOid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kTid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kXid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kCid));
}

TEST(PostgreIO, Integral) {
  {
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(io::WriteBuffer(types, buffer, true));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    bool tgt{false};
    EXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_TRUE(tgt);
  }
  {
    pg::test::Buffer buffer;
    const pg::Smallint src{42};
    EXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    pg::Smallint tgt{0};
    EXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(src, tgt);
  }
  {
    pg::test::Buffer buffer;
    const pg::Integer src{42};
    EXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    pg::Integer tgt{0};
    EXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(src, tgt);
  }
  {
    pg::test::Buffer buffer;
    const pg::Bigint src{42};
    EXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    pg::Bigint tgt{0};
    EXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(src, tgt);
  }
}
