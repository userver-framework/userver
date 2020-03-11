#include <storages/postgres/io/user_types.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

const std::string kCreateTestSchema = "create schema if not exists __pgtest";
const std::string kDropTestSchema = "drop schema if exists __pgtest cascade";

constexpr pg::DBTypeName kEnumName = "__pgtest.rainbow";
const std::string kCreateAnEnumType = R"~(
create type __pgtest.rainbow as enum (
  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
))~";

constexpr pg::DBTypeName kCompositeName = "__pgtest.foobar";
const std::string kCreateACompositeType = R"~(
create type __pgtest.foobar as (
  i integer,
  s text,
  d double precision
))~";

constexpr pg::DBTypeName kRangeName = "__pgtest.timerange";
const std::string kCreateARangeType = R"~(
create type __pgtest.timerange as range(
  subtype = time
))~";

constexpr pg::DBTypeName kDomainName = "__pgtest.dom";
const std::string kCreateADomain = R"~(
create domain __pgtest.dom as text default 'foobar' not null)~";

}  // namespace

/*! [User type] */
namespace pgtest {
struct FooBar {
  pg::Integer i;
  std::string s;
  double d;
};
}  // namespace pgtest
/*! [User type] */
/*! [User type mapping] */
namespace storages::postgres::io {
// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pgtest::FooBar> {
  static constexpr DBTypeName postgres_name = "__pgtest.foobar";
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
  ASSERT_FALSE(res.IsEmpty());
}

POSTGRE_TEST_P(LoadUserTypes) {
  EXPECT_TRUE(io::HasParser(kCompositeName))
      << "Binary parser for composite is registered";

  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";

  const auto& user_types = conn->GetUserTypes();
  EXPECT_EQ(0, user_types.FindOid(kEnumName)) << "Find enumeration type oid";
  EXPECT_EQ(0, user_types.FindOid(kCompositeName)) << "Find composite type oid";
  EXPECT_EQ(0, user_types.FindOid(kRangeName)) << "Find range type oid";

  EXPECT_EQ(0, pg::io::CppToPg<pgtest::FooBar>::GetOid(user_types))
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

  EXPECT_NE(0, io::CppToPg<pgtest::FooBar>::GetOid(user_types))
      << "The type has been created in the database and can be mapped";

  auto enum_oid = user_types.FindOid(kEnumName);
  auto name = user_types.FindName(enum_oid);
  EXPECT_FALSE(name.Empty());

  {
    auto domain_oid = user_types.FindOid(kDomainName);
    auto base_oid = user_types.FindBaseOid(kDomainName);
    EXPECT_NE(0, domain_oid);
    EXPECT_NE(0, base_oid);
    EXPECT_NO_THROW(res = conn->Execute("select 'foo'::__pgtest.dom"));
    auto field = res[0][0];
    EXPECT_EQ(base_oid, field.GetTypeOid());
  }
  {
    // misc domains
    CheckDomainExpectations(
        conn, "create domain __pgtest.int_dom as integer not null",
        "select 1::__pgtest.int_dom");
    CheckDomainExpectations(conn,
                            "create domain __pgtest.real_dom as real not null",
                            "select 1::__pgtest.real_dom");
    CheckDomainExpectations(
        conn, "create domain __pgtest.ts_dom as timestamp not null",
        "select current_timestamp::__pgtest.ts_dom");
  }

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace
