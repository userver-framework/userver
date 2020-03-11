#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

using namespace io::traits;

static_assert(kHasFormatter<pg::Smallint>, "Smallint has a binary formatter");
static_assert(kHasParser<pg::Smallint>, "Smallint has a binary parser");

static_assert(kHasFormatter<pg::Integer>, "Integer has a binary formatter");
static_assert(kHasParser<pg::Integer>, "Integer has a binary parser");

static_assert(kHasFormatter<pg::Bigint>, "Bigint has a binary formatter");
static_assert(kHasParser<pg::Bigint>, "Bigint has a binary parser");

}  // namespace static_test

TEST(PostgreIO, IntegralParserRegistry) {
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt2));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt4));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt8));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kOid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kTid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kXid));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kCid));
}
