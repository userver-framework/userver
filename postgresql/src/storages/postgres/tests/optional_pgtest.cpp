#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/optional.hpp>
#include <userver/storages/postgres/io/string_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

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

using utils_optional_ref_int = utils::OptionalRef<int>;
using utils_optional_ref_string = utils::OptionalRef<std::string>;

static_assert(tt::kIsMappedToPg<utils_optional_ref_int>);
static_assert(tt::kIsMappedToPg<utils_optional_ref_string>);
static_assert(tt::kHasFormatter<utils_optional_ref_int>);
static_assert(tt::kHasFormatter<utils_optional_ref_string>);
static_assert(!tt::kHasParser<utils_optional_ref_int>);
static_assert(!tt::kHasParser<utils_optional_ref_string>);

}  // namespace static_test

namespace {
const pg::UserTypes types;

TEST(PostgreIO, BoostOptional) {
  using optional_int = static_test::boost_optional_int;
  {
    pg::test::Buffer buffer;
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    optional_int null;
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    UEXPECT_NO_THROW(io::WriteRawBinary(types, buffer, null));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    optional_int tgt;
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    UEXPECT_NO_THROW(io::ReadRawBinary(fb, tgt, {}));
    EXPECT_TRUE(!tgt) << "Unexpected value '" << *tgt << "' instead of null ";
    EXPECT_EQ(null, tgt);
  }
}

UTEST_P(PostgreConnection, BoostOptionalRoundtrip) {
  using optional_int = static_test::boost_optional_int;
  using optional_string = static_test::boost_optional_string;
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  {
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    optional_int null;
    optional_int src{42};
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
    optional_int tgt1;
    optional_int tgt2;
    UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
  {
    optional_string null;
    optional_string src{"foobar"};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
    optional_string tgt1;
    optional_string tgt2;
    UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
}

UTEST_P(PostgreConnection, BoostOptionalStored) {
  using optional_int = static_test::boost_optional_int;
  using optional_string = static_test::boost_optional_string;
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  optional_string null{};
  optional_int src{42};
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
  optional_string tgt1;
  optional_int tgt2;
  UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
  EXPECT_EQ(null, tgt1);
  EXPECT_EQ(src, tgt2);
}

TEST(PostgreIO, StdOptional) {
  using optional_int = static_test::std_optional_int;
  {
    pg::test::Buffer buffer;
    optional_int null;
    UEXPECT_NO_THROW(io::WriteRawBinary(types, buffer, null));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    optional_int tgt;
    UEXPECT_NO_THROW(io::ReadRawBinary(fb, tgt, {}));
    EXPECT_TRUE(!tgt) << "Unexpected value '" << *tgt << "' instead of null ";
    EXPECT_EQ(null, tgt);
  }
}

UTEST_P(PostgreConnection, StdOptionalRoundtrip) {
  using optional_int = static_test::std_optional_int;
  using optional_string = static_test::std_optional_string;
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  {
    optional_int null;
    optional_int src{42};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
    optional_int tgt1;
    optional_int tgt2;
    UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
  {
    optional_string null;
    optional_string src{"foobar"};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
    optional_string tgt1;
    optional_string tgt2;
    UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, tgt1);
    EXPECT_EQ(src, tgt2);
  }
}

UTEST_P(PostgreConnection, StdOptionalStored) {
  using optional_int = static_test::std_optional_int;
  using optional_string = static_test::std_optional_string;
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  optional_string null{};
  optional_int src{42};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
  optional_string tgt1;
  optional_int tgt2;
  UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
  EXPECT_EQ(null, tgt1);
  EXPECT_EQ(src, tgt2);
}

TEST(PostgreIO, UtilsOptionalRef) {
  using optional_int = static_test::utils_optional_ref_int;
  using control_optional_int = static_test::std_optional_int;
  {
    pg::test::Buffer buffer;
    optional_int null;
    UEXPECT_NO_THROW(io::WriteRawBinary(types, buffer, null));
    auto fb = pg::test::MakeFieldBuffer(buffer);

    control_optional_int tgt;
    UEXPECT_NO_THROW(io::ReadRawBinary(fb, tgt, {}));
    EXPECT_TRUE(!tgt) << "Unexpected value '" << *tgt << "' instead of null ";
    EXPECT_EQ(null, optional_int{tgt});
  }
}

UTEST_P(PostgreConnection, UtilsOptionalRefRoundtrip) {
  using optional_int = static_test::utils_optional_ref_int;
  using control_optional_int = static_test::std_optional_int;
  using optional_string = static_test::utils_optional_ref_string;
  using control_optional_string = static_test::std_optional_string;
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  {
    int value = 42;

    optional_int null;
    optional_int src{value};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));

    control_optional_int tgt1;
    control_optional_int tgt2;
    UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, optional_int{tgt1});
    EXPECT_EQ(src, optional_int{tgt2});
  }
  {
    std::string value = "foobar";

    optional_string null;
    optional_string src{value};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
    control_optional_string tgt1;
    control_optional_string tgt2;
    UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
    EXPECT_EQ(null, optional_string{tgt1});
    EXPECT_EQ(src, optional_string{tgt2});
  }
}

UTEST_P(PostgreConnection, UtilsOptionalRefStored) {
  using optional_int = static_test::utils_optional_ref_int;
  using control_optional_int = static_test::std_optional_int;
  using optional_string = static_test::utils_optional_ref_string;
  using control_optional_string = static_test::std_optional_string;
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  int value = 42;

  optional_string null{};
  optional_int src{value};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2", null, src));
  control_optional_string tgt1;
  control_optional_int tgt2;
  UEXPECT_NO_THROW(res[0].To(tgt1, tgt2));
  EXPECT_EQ(null, optional_string{tgt1});
  EXPECT_EQ(src, optional_int{tgt2});
}

}  // namespace

USERVER_NAMESPACE_END
