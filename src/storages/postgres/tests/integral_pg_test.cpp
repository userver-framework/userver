#include <gtest/gtest.h>

#include <boost/core/ignore_unused.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

using namespace io::traits;

static_assert(kHasTextFormatter<pg::Smallint>, "Smallint has a text formatter");
static_assert(kHasBinaryFormatter<pg::Smallint>,
              "Smallint has a binary formatter");
static_assert(kHasTextParser<pg::Smallint>, "Smallint has a text parser");
static_assert(kHasBinaryParser<pg::Smallint>, "Smallint has a binary parser");

static_assert(kHasTextFormatter<pg::Integer>, "Integer has a text formatter");
static_assert(kHasBinaryFormatter<pg::Integer>,
              "Integer has a binary formatter");
static_assert(kHasTextParser<pg::Integer>, "Integer has a text parser");
static_assert(kHasBinaryParser<pg::Integer>, "Integer has a binary parser");

static_assert(kHasTextFormatter<pg::Bigint>, "Bigint has a text formatter");
static_assert(kHasBinaryFormatter<pg::Bigint>, "Bigint has a binary formatter");
static_assert(kHasTextParser<pg::Bigint>, "Bigint has a text parser");
static_assert(kHasBinaryParser<pg::Bigint>, "Bigint has a binary parser");

}  // namespace static_test

TEST(PostgreIO, IntegralParserRegistry) {
  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kInt2));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kInt2));

  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kInt4));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kInt4));

  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kInt8));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kInt8));

  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kOid));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kOid));

  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kTid));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kTid));

  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kXid));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kXid));

  EXPECT_TRUE(io::HasTextParser(io::PredefinedOids::kCid));
  EXPECT_TRUE(io::HasBinaryParser(io::PredefinedOids::kCid));
}
