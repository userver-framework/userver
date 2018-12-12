#include <storages/postgres/detail/db_data_type_name.hpp>
#include <storages/postgres/io/enum_types.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

const std::string kCreateTestSchema = "create schema if not exists __pg_test";
const std::string kDropTestSchema = "drop schema if exists __pg_test cascade";

const std::string kCreateAnEnumType = R"~(
create type __pg_test.rainbow as enum (
  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
))~";

const std::string kSelectEnumValues = R"~(
select  'red'::__pg_test.rainbow as red,
        'orange'::__pg_test.rainbow as orange,
        'yellow'::__pg_test.rainbow as yellow,
        'green'::__pg_test.rainbow as green,
        'cyan'::__pg_test.rainbow as cyan,
        'blue'::__pg_test.rainbow as blue,
        'violet'::__pg_test.rainbow as violet
)~";

enum class Rainbow { kRed, kOrange, kYellow, kGreen, kCyan, kBlue, kViolet };

}  // namespace

namespace storages::postgres::io {

template <>
struct CppToUserPg<Rainbow> : EnumMappingBase<Rainbow> {
  static constexpr DBTypeName postgres_name = "__pg_test.rainbow";
  static constexpr EnumeratorList enumerators{
      {EnumType::kRed, "red"},       {EnumType::kOrange, "orange"},
      {EnumType::kYellow, "yellow"}, {EnumType::kGreen, "green"},
      {EnumType::kCyan, "cyan"},     {EnumType::kBlue, "blue"},
      {EnumType::kViolet, "violet"}};
};

}  // namespace storages::postgres::io

namespace static_test {

static_assert(tt::IsMappedToPg<Rainbow>(), "");
static_assert(io::detail::EnumerationMap<Rainbow>::size == 7, "");

static_assert(tt::kHasBinaryParser<Rainbow>, "");
static_assert(tt::kHasBinaryFormatter<Rainbow>, "");
static_assert(tt::kHasTextParser<Rainbow>, "");
static_assert(tt::kHasTextFormatter<Rainbow>, "");

}  // namespace static_test

namespace {

TEST(PostgreIO, Enum) {
  using EnumMap = io::detail::EnumerationMap<Rainbow>;
  EXPECT_EQ("red", EnumMap::GetLiteral(Rainbow::kRed));
  EXPECT_EQ(Rainbow::kRed, EnumMap::GetEnumerator("red"));
}

POSTGRE_TEST_P(EnumRoundtrip) {
  using EnumMap = io::detail::EnumerationMap<Rainbow>;
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";

  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateAnEnumType))
      << "Successfully create an enumeration type";
  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";
  const auto& user_types = conn->GetUserTypes();
  EXPECT_NE(0, io::CppToPg<Rainbow>::GetOid(user_types));

  EXPECT_NO_THROW(res = conn->ExperimentalExecute(
                      kSelectEnumValues, io::DataFormat::kBinaryDataFormat));
  for (auto f : res.Front()) {
    EXPECT_NO_THROW(f.As<Rainbow>());
  }
  EXPECT_NO_THROW(res = conn->ExperimentalExecute(
                      kSelectEnumValues, io::DataFormat::kTextDataFormat));
  for (auto f : res.Front()) {
    EXPECT_NO_THROW(f.As<Rainbow>());
  }

  for (const auto& en : EnumMap::enumerators) {
    EXPECT_NO_THROW(res = conn->Execute("select $1", en.enumerator));
    EXPECT_EQ(en.enumerator, res[0][0].As<Rainbow>());
    EXPECT_EQ(en.literal, res[0][0].As<::utils::string_view>());
  }

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace
