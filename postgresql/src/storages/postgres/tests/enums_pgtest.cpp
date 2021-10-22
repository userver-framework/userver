#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/detail/db_data_type_name.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

const std::string kCreateTestSchema = "create schema if not exists __pgtest";
const std::string kDropTestSchema = "drop schema if exists __pgtest cascade";

/*! [Enum type DDL] */
const std::string kCreateAnEnumType = R"~(
create type __pgtest.rainbow as enum (
  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
))~";
/*! [Enum type DDL] */

const std::string kSelectEnumValues = R"~(
select  'red'::__pgtest.rainbow as red,
        'orange'::__pgtest.rainbow as orange,
        'yellow'::__pgtest.rainbow as yellow,
        'green'::__pgtest.rainbow as green,
        'cyan'::__pgtest.rainbow as cyan,
        'blue'::__pgtest.rainbow as blue,
        'violet'::__pgtest.rainbow as violet
)~";

/*! [C++ enum type] */
enum class Rainbow { kRed, kOrange, kYellow, kGreen, kCyan, kBlue, kViolet };
/*! [C++ enum type] */

// This data type is for testing a data type that is used only for reading
enum class RainbowRO { kRed, kOrange, kYellow, kGreen, kCyan, kBlue, kViolet };

}  // namespace

// this is used as a code snippet in documentation, clang-format makes it ugly
// clang-format off
/*! [C++ to Pg mapping] */
namespace storages::postgres::io {
// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<Rainbow> : EnumMappingBase<Rainbow> {
  static constexpr DBTypeName postgres_name = "__pgtest.rainbow";
  static constexpr EnumeratorList enumerators{
      {EnumType::kRed,    "red"},
      {EnumType::kOrange, "orange"},
      {EnumType::kYellow, "yellow"},
      {EnumType::kGreen,  "green"},
      {EnumType::kCyan,   "cyan"},
      {EnumType::kBlue,   "blue"},
      {EnumType::kViolet, "violet"}};
};
}  // namespace storages::postgres::io
/*! [C++ to Pg mapping] */
// clang-format on

// Reopen the namespace not to get to the code snippet
namespace storages::postgres::io {

template <>
struct CppToUserPg<RainbowRO> : EnumMappingBase<RainbowRO> {
  static constexpr DBTypeName postgres_name = "__pgtest.rainbow";
  static constexpr EnumeratorList enumerators{
      {EnumType::kRed, "red"},       {EnumType::kOrange, "orange"},
      {EnumType::kYellow, "yellow"}, {EnumType::kGreen, "green"},
      {EnumType::kCyan, "cyan"},     {EnumType::kBlue, "blue"},
      {EnumType::kViolet, "violet"}};
};

namespace traits {

// To ensure it is never written to a buffer
template <>
struct HasFormatter<RainbowRO> : std::false_type {};

}  // namespace traits

}  // namespace storages::postgres::io

namespace static_test {

static_assert(tt::IsMappedToPg<Rainbow>());
static_assert(io::detail::EnumerationMap<Rainbow>::size == 7);

static_assert(tt::kHasParser<Rainbow>);
static_assert(tt::kHasFormatter<Rainbow>);

}  // namespace static_test

namespace {

TEST(PostgreIO, Enum) {
  using EnumMap = io::detail::EnumerationMap<Rainbow>;
  EXPECT_EQ("red", EnumMap::GetLiteral(Rainbow::kRed));
  EXPECT_EQ(Rainbow::kRed, EnumMap::GetEnumerator("red"));
}

UTEST_F(PostgreConnection, EnumRoundtrip) {
  using EnumMap = io::detail::EnumerationMap<Rainbow>;
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";

  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateAnEnumType))
      << "Successfully create an enumeration type";
  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";
  const auto& user_types = conn->GetUserTypes();
  EXPECT_NE(0, io::CppToPg<Rainbow>::GetOid(user_types));

  EXPECT_NO_THROW(res = conn->Execute(kSelectEnumValues));
  for (auto f : res.Front()) {
    EXPECT_NO_THROW(f.As<Rainbow>());
  }

  for (const auto& en : EnumMap::enumerators) {
    EXPECT_NO_THROW(res = conn->Execute("select $1", en.enumerator));
    EXPECT_EQ(en.enumerator, res[0][0].As<Rainbow>());
    EXPECT_EQ(en.literal, res[0][0].As<std::string_view>());
    // Test the data type that is used for reading only
    EXPECT_NO_THROW(res[0][0].As<RainbowRO>())
        << "Read a datatype that is never written to a Pg buffer";
  }

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace

USERVER_NAMESPACE_END
