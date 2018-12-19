#include <storages/postgres/io/user_types.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

const std::string kCreateTestSchema = "create schema if not exists __pg_test";
const std::string kDropTestSchema = "drop schema if exists __pg_test cascade";

constexpr pg::DBTypeName kEnumName = "__pg_test.rainbow";
const std::string kCreateAnEnumType = R"~(
create type __pg_test.rainbow as enum (
  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
))~";

constexpr pg::DBTypeName kCompositeName = "__pg_test.foobar";
const std::string kCreateACompositeType = R"~(
create type __pg_test.foobar as (
  i integer,
  s text,
  d double precision
))~";

constexpr pg::DBTypeName kRangeName = "__pg_test.timerange";
const std::string kCreateARangeType = R"~(
create type __pg_test.timerange as range(
  subtype = time
))~";

constexpr pg::DBTypeName kDomainName = "__pg_test.dom";
const std::string kCreateADomain = R"~(
create domain __pg_test.dom as text default 'foobar' not null)~";

}  // namespace

/*! [User type] */
namespace pg_test {
struct FooBar {
  pg::Integer i;
  std::string s;
  double d;
};
}  // namespace pg_test
/*! [User type] */
/*! [User type mapping] */
namespace storages::postgres::io {
// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pg_test::FooBar> {
  static constexpr DBTypeName postgres_name = "__pg_test.foobar";
};
}  // namespace storages::postgres::io
/*! [User type mapping] */

namespace {

void CheckDomainExpectations(pg::detail::ConnectionPtr& conn,
                             const std::string& create_statement,
                             const std::string& check_statement) {
  EXPECT_NO_THROW(conn->Execute(create_statement)) << create_statement;
  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute(check_statement)) << check_statement;
  ASSERT_TRUE(res);
  EXPECT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
}

POSTGRE_TEST_P(LoadUserTypes) {
  EXPECT_TRUE(io::HasBinaryParser(kCompositeName))
      << "Binary parser for composite is registered";
  EXPECT_FALSE(io::HasTextParser(kCompositeName))
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
  EXPECT_NO_THROW(conn->Execute(kCreateADomain))
      << "Successfully create a domain";

  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";

  EXPECT_NE(0, user_types.FindOid(kEnumName)) << "Find enumeration type oid";
  EXPECT_NE(0, user_types.FindOid(kCompositeName)) << "Find composite type oid";
  EXPECT_NE(0, user_types.FindOid(kRangeName)) << "Find range type oid";
  EXPECT_NE(0, user_types.FindOid(kDomainName)) << "Find domain type oid";

  EXPECT_NE(0, io::CppToPg<pg_test::FooBar>::GetOid(user_types))
      << "The type has been created in the database and can be mapped";

  auto enum_oid = user_types.FindOid(kEnumName);
  auto name = user_types.FindName(enum_oid);
  EXPECT_FALSE(name.Empty());

  {
    auto domain_oid = user_types.FindOid(kDomainName);
    auto base_oid = user_types.FindBaseOid(kDomainName);
    EXPECT_NE(0, domain_oid);
    EXPECT_NE(0, base_oid);
    EXPECT_NO_THROW(res = conn->Execute("select 'foo'::__pg_test.dom"));
    auto field = res[0][0];
    EXPECT_EQ(base_oid, field.GetTypeOid());
    EXPECT_EQ(io::DataFormat::kBinaryDataFormat, field.GetDataFormat());
  }
  {
    // misc domains
    CheckDomainExpectations(
        conn, "create domain __pg_test.int_dom as integer not null",
        "select 1::__pg_test.int_dom");
    CheckDomainExpectations(conn,
                            "create domain __pg_test.real_dom as real not null",
                            "select 1::__pg_test.real_dom");
    CheckDomainExpectations(
        conn, "create domain __pg_test.ts_dom as timestamp not null",
        "select current_timestamp::__pg_test.ts_dom");
  }

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace
