#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/detail/db_data_type_name.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/utils/trivial_map.hpp>

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
  'red', 'orange', 'yellow', 'green', 'cyan'
))~";
/*! [Enum type DDL] */

const std::string kSelectEnumValues = R"~(
select  'red'::__pgtest.rainbow as red,
        'orange'::__pgtest.rainbow as orange,
        'yellow'::__pgtest.rainbow as yellow,
        'green'::__pgtest.rainbow as green,
        'cyan'::__pgtest.rainbow as cyan
)~";

/*! [C++ enum type] */
enum class Rainbow { kRed, kOrange, kYellow, kGreen, kCyan };
enum class AnotherRainbow { kRed, kOrange, kYellow, kGreen, kCyan };
/*! [C++ enum type] */

// This data type is for testing a data type that is used only for reading
enum class RainbowRO { kRed, kOrange, kYellow, kGreen, kCyan };

enum class AnotherRainbowRO { kRed, kOrange, kYellow, kGreen, kCyan };

}  // namespace

// this is used as a code snippet in documentation, clang-format makes it ugly
// clang-format off
/*! [C++ to Pg mapping] */
namespace storages::postgres::io {
// This specialization MUST go to the header together with the mapped type
template <>
struct CppToUserPg<Rainbow> : EnumMappingBase<Rainbow> {
  static constexpr DBTypeName postgres_name = "__pgtest.rainbow";
  static constexpr Enumerator enumerators[] {
      {EnumType::kRed,    "red"},
      {EnumType::kOrange, "orange"},
      {EnumType::kYellow, "yellow"},
      {EnumType::kGreen,  "green"},
      {EnumType::kCyan,   "cyan"}};
};
}  // namespace storages::postgres::io
/*! [C++ to Pg mapping] */
// clang-format on

// this is used as a code snippet in documentation, clang-format makes it ugly
// clang-format off
/*! [C++ to Pg TrivialBiMap mapping] */
namespace storages::postgres::io {
// This specialization MUST go to the header together with the mapped type
template <>
struct CppToUserPg<AnotherRainbow> : EnumMappingBase<AnotherRainbow> {
  static constexpr DBTypeName postgres_name = "__pgtest.rainbow";
  static constexpr USERVER_NAMESPACE::utils::TrivialBiMap enumerators =
      [](auto selector) {
          return selector()
              .Case("red", AnotherRainbow::kRed)
              .Case("orange", AnotherRainbow::kOrange)
              .Case("yellow", AnotherRainbow::kYellow)
              .Case("green", AnotherRainbow::kGreen)
              .Case("cyan", AnotherRainbow::kCyan);
      };
};
}  // namespace storages::postgres::io
/*! [C++ to Pg TrivialBiMap mapping] */
// clang-format on

// Reopen the namespace not to get to the code snippet
namespace storages::postgres::io {

template <>
struct CppToUserPg<RainbowRO> : EnumMappingBase<RainbowRO> {
  static constexpr DBTypeName postgres_name = "__pgtest.rainbow";
  static constexpr Enumerator enumerators[]{{EnumType::kRed, "red"},
                                            {EnumType::kOrange, "orange"},
                                            {EnumType::kYellow, "yellow"},
                                            {EnumType::kGreen, "green"},
                                            {EnumType::kCyan, "cyan"}};
};

template <>
struct CppToUserPg<AnotherRainbowRO> : EnumMappingBase<AnotherRainbowRO> {
  static constexpr DBTypeName postgres_name = "__pgtest.rainbow";
  static constexpr USERVER_NAMESPACE::utils::TrivialBiMap enumerators =
      [](auto selector) {
        return selector()
            .Case("red", AnotherRainbowRO::kRed)
            .Case("orange", AnotherRainbowRO::kOrange)
            .Case("yellow", AnotherRainbowRO::kYellow)
            .Case("green", AnotherRainbowRO::kGreen)
            .Case("cyan", AnotherRainbowRO::kCyan);
      };
};

namespace traits {

// To ensure it is never written to a buffer
template <>
struct HasFormatter<RainbowRO> : std::false_type {};

template <>
struct HasFormatter<AnotherRainbowRO> : std::false_type {};

}  // namespace traits

}  // namespace storages::postgres::io

namespace static_test {

template <typename T>
struct Checker {
  static_assert(tt::IsMappedToPg<T>());
  static_assert(io::detail::EnumerationMap<T>::size == 5);

  static_assert(tt::kHasParser<T>);
  static_assert(tt::kHasFormatter<T>);
};
template struct Checker<Rainbow>;
template struct Checker<AnotherRainbow>;

}  // namespace static_test

namespace {

TEST(PostgreIO, Enum) {
  using EnumMap = io::detail::EnumerationMap<Rainbow>;
  EXPECT_EQ("red", EnumMap::GetLiteral(Rainbow::kRed));
  EXPECT_EQ(Rainbow::kRed, EnumMap::GetEnumerator("red"));
}

TEST(PostgreIO, EnumTrivialMap) {
  using EnumMap = io::detail::EnumerationMap<AnotherRainbow>;
  EXPECT_EQ("red", EnumMap::GetLiteral(AnotherRainbow::kRed));
  EXPECT_EQ(AnotherRainbow::kRed, EnumMap::GetEnumerator("red"));
}

UTEST_P(PostgreConnection, EnumRoundtrip) {
  using EnumMap = io::detail::EnumerationMap<Rainbow>;
  CheckConnection(GetConn());
  ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  UASSERT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";

  UASSERT_NO_THROW(GetConn()->Execute(kCreateTestSchema)) << "Create schema";

  UEXPECT_NO_THROW(GetConn()->Execute(kCreateAnEnumType))
      << "Successfully create an enumeration type";
  UEXPECT_NO_THROW(GetConn()->ReloadUserTypes()) << "Reload user types";
  const auto& user_types = GetConn()->GetUserTypes();
  EXPECT_NE(0, io::CppToPg<Rainbow>::GetOid(user_types));

  UEXPECT_NO_THROW(res = GetConn()->Execute(kSelectEnumValues));
  for (auto f : res.Front()) {
    UEXPECT_NO_THROW(f.As<Rainbow>());
  }

  for (const auto& en : EnumMap::enumerators) {
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", en.enumerator));
    EXPECT_EQ(en.enumerator, res[0][0].As<Rainbow>());
    EXPECT_EQ(en.literal, res[0][0].As<std::string_view>());
    // Test the data type that is used for reading only
    UEXPECT_NO_THROW(res[0][0].As<RainbowRO>())
        << "Read a datatype that is never written to a Pg buffer";
  }

  UEXPECT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
}

UTEST_P(PostgreConnection, EnumTrivialMapRoundtrip) {
  using EnumMap = io::detail::EnumerationMap<AnotherRainbow>;
  CheckConnection(GetConn());
  ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  UASSERT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";

  UASSERT_NO_THROW(GetConn()->Execute(kCreateTestSchema)) << "Create schema";

  UEXPECT_NO_THROW(GetConn()->Execute(kCreateAnEnumType))
      << "Successfully create an enumeration type";
  UEXPECT_NO_THROW(GetConn()->ReloadUserTypes()) << "Reload user types";
  const auto& user_types = GetConn()->GetUserTypes();
  EXPECT_NE(0, io::CppToPg<AnotherRainbow>::GetOid(user_types));

  UEXPECT_NO_THROW(res = GetConn()->Execute(kSelectEnumValues));
  for (auto f : res.Front()) {
    UEXPECT_NO_THROW(f.As<AnotherRainbow>());
  }

  static constexpr AnotherRainbow enumerators[]{
      AnotherRainbow::kRed, AnotherRainbow::kOrange, AnotherRainbow::kYellow,
      AnotherRainbow::kGreen, AnotherRainbow::kCyan};
  for (const auto& en : enumerators) {
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", en));
    EXPECT_EQ(en, res[0][0].As<AnotherRainbow>());
    EXPECT_EQ(EnumMap::GetLiteral(en), res[0][0].As<std::string_view>());
    // Test the data type that is used for reading only
    UEXPECT_NO_THROW(res[0][0].As<AnotherRainbowRO>())
        << "Read a datatype that is never written to a Pg buffer";
  }

  UEXPECT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace

USERVER_NAMESPACE_END
