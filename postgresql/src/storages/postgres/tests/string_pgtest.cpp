#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

namespace tt = io::traits;

static_assert(tt::kHasFormatter<std::string>);
static_assert(tt::kHasParser<std::string>);

static_assert(tt::kIsMappedToPg<std::string>);

static_assert(tt::kHasFormatter<const char*>);

static_assert(tt::kIsMappedToPg<const char*>);

static_assert(tt::kHasFormatter<char[5]>);

static_assert(tt::kIsMappedToPg<char[5]>);

static_assert(tt::kHasFormatter<char>);
static_assert(tt::kHasParser<char>);

static_assert(tt::kIsMappedToPg<char>);

}  // namespace static_test

namespace {

TEST(PostgreIO, StringParserRegistry) {
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kChar));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kText));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kBpchar));
  EXPECT_TRUE(io::HasParser(io::PredefinedOids::kVarchar));
}

UTEST_P(PostgreConnection, StringRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  std::string unicode_str{"юникод µ𝞪∑∆ƒæ©⩜"};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", unicode_str));
  EXPECT_EQ(unicode_str, res[0][0].As<std::string>());
  auto str_res = res.AsSetOf<std::string>();
  EXPECT_EQ(unicode_str, str_res[0]);

  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", std::string{}));
  EXPECT_EQ(std::string{}, res[0][0].As<std::string>()) << "Empty string";
}

UTEST_P(PostgreConnection, StringStored) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  std::string std_str = "std::string";
  constexpr auto c_str = "const char*";
  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select $1, $2",
                       pg::ParameterStore{}.PushBack(std_str).PushBack(c_str)));
  EXPECT_EQ(std_str, res[0][0].As<std::string>());
  EXPECT_EQ(c_str, res[0][1].As<std::string>());
}

}  // namespace

USERVER_NAMESPACE_END
