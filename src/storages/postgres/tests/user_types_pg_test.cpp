#include <storages/postgres/io/user_types.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;

namespace {

constexpr const char* const kSchemaName = "__pg_test";
const std::string kCreateTestSchema = "create schema if not exists __pg_test";
const std::string kDropTestSchema = "drop schema if exists __pg_test cascade";

constexpr pg::DBTypeName kEnumName{kSchemaName, "rainbow"};
const std::string kCreateAnEnumType = R"~(
create type __pg_test.rainbow as enum (
  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
))~";

constexpr pg::DBTypeName kCompositeName{kSchemaName, "foobar"};
const std::string kCreateACompositeType = R"~(
create type __pg_test.foobar as (
  i integer,
  s text,
  d double precision
))~";

constexpr pg::DBTypeName kRangeName{kSchemaName, "timerange"};
const std::string kCreateARangeType = R"~(
create type __pg_test.timerange as range(
  subtype = time
))~";

constexpr pg::DBTypeName kDomainName{kSchemaName, "dom"};
const std::string kCreateADomain = R"~()~";

constexpr pg::DBTypeName kNotThere{kSchemaName, "not_there"};

}  // namespace

namespace pg_test {

struct FooBar {
  pg::Integer i;
  std::string s;
  double d;
};

}  // namespace pg_test

namespace storages::postgres::io {

template <>
struct BufferParser<pg_test::FooBar, DataFormat::kBinaryDataFormat> {};

// User type test registration
template <>
struct CppToUserPg<pg_test::FooBar> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

}  // namespace storages::postgres::io

namespace {

POSTGRE_TEST_P(LoadUserTypes) {
  EXPECT_TRUE(pg::io::HasBinaryParser(kCompositeName))
      << "Binary parser for composite is registered";
  EXPECT_FALSE(pg::io::HasTextParser(kCompositeName))
      << "Text parser for composite is not registered";

  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";

  const auto& user_types = conn->GetUserTypes();
  EXPECT_EQ(0, user_types.FindOid(kEnumName)) << "Find enumeration type oid";
  EXPECT_EQ(0, user_types.FindOid(kCompositeName)) << "Find composite type oid";
  EXPECT_EQ(0, user_types.FindOid(kRangeName)) << "Find range type oid";

  EXPECT_EQ(0, pg::io::CppToPg<pg_test::FooBar>::GetOid(user_types))
      << "The type is not in the database yet";

  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateAnEnumType))
      << "Successfully create an enumeration type";
  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  EXPECT_NO_THROW(conn->Execute(kCreateARangeType))
      << "Successfully create a range type";

  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";

  EXPECT_NE(0, user_types.FindOid(kEnumName)) << "Find enumeration type oid";
  EXPECT_NE(0, user_types.FindOid(kCompositeName)) << "Find composite type oid";
  EXPECT_NE(0, user_types.FindOid(kRangeName)) << "Find range type oid";

  EXPECT_NE(0, pg::io::CppToPg<pg_test::FooBar>::GetOid(user_types))
      << "The type has been created in the database and can be mapped";

  auto enum_oid = user_types.FindOid(kEnumName);
  auto name = user_types.FindName(enum_oid);
  EXPECT_FALSE(name.Empty());

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace
