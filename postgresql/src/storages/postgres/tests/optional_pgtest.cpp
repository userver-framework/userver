#include <boost/optional.hpp>

#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/optional.hpp>
#include <storages/postgres/io/string_types.hpp>
#include <storages/postgres/parameter_store.hpp>
#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace static_test {

using boost_optional_int = boost::optional<int>;
using boost_optional_string = boost::optional<std::string>;

static_assert(tt::kIsMappedToPg<boost_optional_int>);
static_assert(tt::kIsMappedToPg<boost_optional_string>);

using std_optional_int = std::optional<int>;
using std_optional_string = std::optional<std::string>;

static_assert(tt::kIsMappedToPg<std_optional_int>);
static_assert(tt::kIsMappedToPg<std_optional_string>);

}  // namespace static_test

namespace {

const pg::UserTypes types;

TEST(PostgreIO, BoostOptional) {
  using optional_int = static_test::boost_optional_int;
  {
    pg::test::Buffer buffer;
    optional_int null;
    EXPECT_NO_THROW(io::WriteRawBinary(types, buffer, null));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    optional_int tgt;
    EXPECT_NO_THROW(io::ReadRawBinary(fb, tgt, {}));
    EXPECT_TRUE(!tgt) << "Unexpected value '" << *tgt << "' instead of null ";
    EXPECT_EQ(null, tgt);
  }
}

POSTGRE_TEST_P(BoostOptionalRoundtrip) {
  using optional_int = static_test::boost_optional_int;
  using optional_string = static_test::boost_optional_string;
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};

  {
    optional_int null;
    optional_int src{42};
    EXPECT_NO_THROW(res = conn->Execute("select $1, $2", null, src));
    optional_int tgt1, tgt2;
    EXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
  {
    optional_string null;
    optional_string src{"foobar"};
    EXPECT_NO_THROW(res = conn->Execute("select $1, $2", null, src));
    optional_string tgt1, tgt2;
    EXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
}

POSTGRE_TEST_P(BoostOptionalStored) {
  using optional_int = static_test::boost_optional_int;
  using optional_string = static_test::boost_optional_string;
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};

  optional_string null{};
  optional_int src{42};
  EXPECT_NO_THROW(res = conn->Execute("select $1, $2", null, src));
  optional_string tgt1;
  optional_int tgt2;
  EXPECT_NO_THROW(res[0].To(tgt1, tgt2));
  EXPECT_EQ(null, tgt1);
  EXPECT_EQ(src, tgt2);
}

TEST(PostgreIO, StdOptional) {
  using optional_int = static_test::std_optional_int;
  {
    pg::test::Buffer buffer;
    optional_int null;
    EXPECT_NO_THROW(io::WriteRawBinary(types, buffer, null));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    optional_int tgt;
    EXPECT_NO_THROW(io::ReadRawBinary(fb, tgt, {}));
    EXPECT_TRUE(!tgt) << "Unexpected value '" << *tgt << "' instead of null ";
    EXPECT_EQ(null, tgt);
  }
}

POSTGRE_TEST_P(StdOptionalRoundtrip) {
  using optional_int = static_test::std_optional_int;
  using optional_string = static_test::std_optional_string;
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};

  {
    optional_int null;
    optional_int src{42};
    EXPECT_NO_THROW(res = conn->Execute("select $1, $2", null, src));
    optional_int tgt1, tgt2;
    EXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
  {
    optional_string null;
    optional_string src{"foobar"};
    EXPECT_NO_THROW(res = conn->Execute("select $1, $2", null, src));
    optional_string tgt1, tgt2;
    EXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
}

POSTGRE_TEST_P(StdOptionalStored) {
  using optional_int = static_test::std_optional_int;
  using optional_string = static_test::std_optional_string;
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};

  optional_string null{};
  optional_int src{42};
  EXPECT_NO_THROW(res = conn->Execute("select $1, $2", null, src));
  optional_string tgt1;
  optional_int tgt2;
  EXPECT_NO_THROW(res[0].To(tgt1, tgt2));
  EXPECT_EQ(null, tgt1);
  EXPECT_EQ(src, tgt2);
}

}  // namespace
