#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

namespace tt = io::traits;

const pg::UserTypes types;

template <typename Int>
class PostgreIOIntegral : public ::testing::Test {};

using IntTypes = ::testing::Types<pg::Smallint, pg::Integer, pg::Bigint, short,
                                  int, long, long long>;

}  // namespace

TEST(PostgreIOIntegral, ParserRegistry) {
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt2));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt4));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt8));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kOid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kTid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kXid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kCid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kLsn));
}

TEST(PostgreIOIntegral, Bool) {
  pg::test::Buffer buffer;
  UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, true));
  auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
  bool tgt{false};
  UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
  EXPECT_TRUE(tgt);
}

TYPED_TEST_SUITE(PostgreIOIntegral, IntTypes);

TYPED_TEST(PostgreIOIntegral, Int) {
  static_assert(tt::kIsMappedToPg<TypeParam>, "missing mapping");
  static_assert(tt::kHasFormatter<TypeParam>, "missing binary formatter");
  static_assert(tt::kHasParser<TypeParam>, "missing binary parser");

  pg::test::Buffer buffer;
  const TypeParam src{42};
  UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
  auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
  TypeParam tgt{0};
  UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
  EXPECT_EQ(src, tgt);
}

USERVER_NAMESPACE_END
