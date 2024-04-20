#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <userver/utils/strong_typedef.hpp>
#include <userver/utils/time_of_day.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

const std::string kCreateTestSchema = "create schema if not exists __pgtest";
const std::string kDropTestSchema = "drop schema if exists __pgtest cascade";

constexpr pg::DBTypeName kEnumName = "__pgtest.rainbow";
const std::string kCreateAnEnumType = R"~(
CREATE TYPE __pgtest.rainbow AS enum (
  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
))~";

constexpr pg::DBTypeName kCompositeName = "__pgtest.composite";
const std::string kCreateACompositeType = R"~(
-- /// [User composite type postgres]
CREATE TYPE __pgtest.composite AS (
  i integer,
  s text,
  d double precision
)
-- /// [User composite type postgres]
)~";

constexpr pg::DBTypeName kRangeName = "__pgtest.timerange";
const std::string kCreateARangeType = R"~(
-- /// [User timerange type postgres]
create type __pgtest.timerange as range(
  subtype = time
)
-- /// [User timerange type postgres]
)~";

constexpr pg::DBTypeName kDomainName = "__pgtest.dom";
const std::string kCreateADomain = R"~(
-- /// [User domain type postgres]
CREATE DOMAIN __pgtest.dom AS integer
  DEFAULT 42
  NOT NULL
-- /// [User domain type postgres]
)~";

constexpr pg::DBTypeName kRangeOverDomainName = "__pgtest.my_range";
const std::string kCreateRangeOverDomain = R"~(
-- /// [User domainrange type postgres]
CREATE TYPE __pgtest.my_range AS RANGE (
  subtype = __pgtest.dom
)
-- /// [User domainrange type postgres]
)~";

}  // namespace

/*! [User composite type cpp] */
namespace pgtest {

struct Composite {
  pg::Integer i;
  std::string s;
  double d;
};

}  // namespace pgtest

// This specialization MUST go to the header together with the mapped type
template <>
struct storages::postgres::io::CppToUserPg<pgtest::Composite> {
  static constexpr DBTypeName postgres_name = "__pgtest.composite";
};
/*! [User composite type cpp] */

/*! [User domainrange type cpp] */
namespace pgtest {

using MyDomain = utils::StrongTypedef<struct MyDomainTag, int>;
using MyRange = storages::postgres::Range<MyDomain>;

}  // namespace pgtest

template <>
struct storages::postgres::io::CppToUserPg<pgtest::MyRange> {
  static constexpr DBTypeName postgres_name = "__pgtest.my_range";
};
/*! [User domainrange type cpp] */

/*! [User timerange type cpp] */
namespace pgtest {

template <typename Duration>
using TimeRange =
    utils::StrongTypedef<struct MyTimeTag,
                         pg::Range<utils::datetime::TimeOfDay<Duration>>>;

template <typename Duration>
using BoundedTimeRange = utils::StrongTypedef<
    struct MyTimeTag, pg::BoundedRange<utils::datetime::TimeOfDay<Duration>>>;

}  // namespace pgtest

template <typename Duration>
struct storages::postgres::io::CppToUserPg<pgtest::TimeRange<Duration>> {
  static constexpr DBTypeName postgres_name = "__pgtest.timerange";
};

template <typename Duration>
struct storages::postgres::io::CppToUserPg<pgtest::BoundedTimeRange<Duration>> {
  static constexpr DBTypeName postgres_name = "__pgtest.timerange";
};
/*! [User timerange type cpp] */

namespace {

void CheckDomainExpectations(pg::detail::ConnectionPtr& conn,
                             const std::string& create_statement,
                             const std::string& check_statement) {
  UEXPECT_NO_THROW(conn->Execute(create_statement)) << create_statement;
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = conn->Execute(check_statement)) << check_statement;
  ASSERT_FALSE(res.IsEmpty());
}

UTEST_P(PostgreConnection, LoadUserTypes) {
  EXPECT_TRUE(io::HasParser(kCompositeName))
      << "Binary parser for composite is registered";

  CheckConnection(GetConn());
  ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";
  UEXPECT_THROW(GetConn()->GetUserTypes().CheckRegisteredTypes(),
                pg::UserTypeError);

  pg::ResultSet res{nullptr};
  UASSERT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
  UEXPECT_NO_THROW(GetConn()->ReloadUserTypes()) << "Reload user types";

  const auto& user_types = GetConn()->GetUserTypes();
  EXPECT_EQ(0, user_types.FindOid(kEnumName)) << "Find enumeration type oid";
  EXPECT_EQ(0, user_types.FindOid(kCompositeName)) << "Find composite type oid";
  EXPECT_EQ(0, user_types.FindOid(kRangeName)) << "Find range type oid";
  EXPECT_EQ(0, user_types.FindOid(kRangeOverDomainName));

  EXPECT_EQ(0, pg::io::CppToPg<pgtest::Composite>::GetOid(user_types))
      << "The type is not in the database yet";

  UASSERT_NO_THROW(GetConn()->Execute(kCreateTestSchema)) << "Create schema";

  UEXPECT_NO_THROW(GetConn()->Execute(kCreateAnEnumType))
      << "Successfully create an enumeration type";
  UEXPECT_NO_THROW(GetConn()->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  UEXPECT_NO_THROW(GetConn()->Execute(kCreateARangeType))
      << "Successfully create a range type";
  UEXPECT_NO_THROW(GetConn()->Execute(kCreateADomain))
      << "Successfully create a domain";
  UEXPECT_NO_THROW(GetConn()->Execute(kCreateRangeOverDomain));

  UEXPECT_NO_THROW(GetConn()->ReloadUserTypes()) << "Reload user types";

  EXPECT_NE(0, user_types.FindOid(kEnumName)) << "Find enumeration type oid";
  EXPECT_NE(0, user_types.FindOid(kCompositeName)) << "Find composite type oid";
  EXPECT_NE(0, user_types.FindOid(kRangeName)) << "Find range type oid";
  EXPECT_NE(0, user_types.FindOid(kDomainName)) << "Find domain type oid";
  EXPECT_NE(0, user_types.FindOid(kRangeOverDomainName));

  EXPECT_NE(0, io::CppToPg<pgtest::Composite>::GetOid(user_types))
      << "The type has been created in the database and can be mapped";

  auto enum_oid = user_types.FindOid(kEnumName);
  auto name = user_types.FindName(enum_oid);
  EXPECT_FALSE(name.Empty());

  {
    auto domain_oid = user_types.FindOid(kDomainName);
    auto base_oid = user_types.FindBaseOid(kDomainName);
    EXPECT_NE(0, domain_oid);
    EXPECT_NE(0, base_oid);
    UEXPECT_NO_THROW(res = GetConn()->Execute("select 4::__pgtest.dom"));
    auto field = res[0][0];
    EXPECT_EQ(base_oid, field.GetTypeOid());
    EXPECT_EQ(field.As<int>(), 4);
  }
  {
    auto& connection = GetConn();
    /// [User domain type cpp usage]
    auto res = connection->Execute("SELECT 5::__pgtest.dom");
    EXPECT_EQ(res[0][0].As<int>(), 5);
    /// [User domain type cpp usage]
  }
  {
    auto& connection = GetConn();
    /// [User composite type cpp usage]
    auto res =
        connection->Execute("SELECT $1", pgtest::Composite{1, "hello", 4.0});

    auto composite = res[0][0].As<pgtest::Composite>();
    EXPECT_EQ(composite.i, 1);
    EXPECT_EQ(composite.s, "hello");
    /// [User composite type cpp usage]
  }
  {
    // misc domains
    CheckDomainExpectations(
        GetConn(), "create domain __pgtest.int_dom as integer not null",
        "select 1::__pgtest.int_dom");
    UEXPECT_NO_THROW(
        GetConn()->Execute("create temp table int_dom_table("
                           "v __pgtest.int_dom)"));
    UEXPECT_NO_THROW(
        GetConn()->Execute("insert into int_dom_table(v) values ($1)", 100500));
    CheckDomainExpectations(GetConn(),
                            "create domain __pgtest.real_dom as real not null",
                            "select 1::__pgtest.real_dom");
    CheckDomainExpectations(
        GetConn(), "create domain __pgtest.ts_dom as timestamp not null",
        "select current_timestamp::__pgtest.ts_dom");
  }

  UEXPECT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
}

UTEST_P(PostgreConnection, UserDefinedRange) {
  CheckConnection(GetConn());
  ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

  using Seconds = utils::datetime::TimeOfDay<std::chrono::seconds>;
  using TimeRange = pgtest::TimeRange<std::chrono::seconds>;
  using BoundedTimeRange = pgtest::BoundedTimeRange<std::chrono::seconds>;
  UEXPECT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
  UASSERT_NO_THROW(GetConn()->Execute(kCreateTestSchema)) << "Create schema";
  UASSERT_NO_THROW(GetConn()->Execute(kCreateARangeType))
      << "Create range type";
  UASSERT_NO_THROW(GetConn()->Execute(kCreateADomain));
  UASSERT_NO_THROW(GetConn()->Execute(kCreateRangeOverDomain));
  UASSERT_NO_THROW(GetConn()->ReloadUserTypes());

  GetConn()->Execute("select '[00:00:01, 00:00:02]'::__pgtest.timerange");
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1",
                               TimeRange{Seconds{std::chrono::seconds{1}},
                                         Seconds{std::chrono::seconds{2}}}));
  BoundedTimeRange tr{};
  UEXPECT_NO_THROW(tr = res.AsSingleRow<BoundedTimeRange>());
  EXPECT_EQ(Seconds(std::chrono::seconds{1}),
            utils::UnderlyingValue(tr).GetLowerBound());
  EXPECT_EQ(Seconds(std::chrono::seconds{2}),
            utils::UnderlyingValue(tr).GetUpperBound());
  EXPECT_TRUE(utils::UnderlyingValue(tr).IsLowerBoundIncluded())
      << "By default a range is lower-bound inclusive";
  ;
  EXPECT_FALSE(utils::UnderlyingValue(tr).IsUpperBoundIncluded())
      << "By default a range is upper-bound exclusive";
  {
    auto& connection = GetConn();
    /// [User domainrange type usage]
    auto result =
        // connection->Execute("select $1", pgtest::MyRange{pgtest::MyDomain{1},
        // pgtest::MyDomain{2}});
        connection->Execute("select '[1, 2]'::__pgtest.my_range");
    auto range = result[0][0].As<pgtest::MyRange>();
    EXPECT_EQ(pgtest::MyDomain{1}, range.GetLowerBound());
    EXPECT_EQ(pgtest::MyDomain{2}, range.GetUpperBound());
    /// [User domainrange type usage]
  }

  {
    auto& connection = GetConn();
    /// [User timerange type usage]
    using namespace std::chrono_literals;

    using Seconds = utils::datetime::TimeOfDay<std::chrono::seconds>;
    using TimeRange = pgtest::TimeRange<std::chrono::seconds>;
    using BoundedTimeRange = pgtest::BoundedTimeRange<std::chrono::seconds>;

    auto result =
        connection->Execute("select $1", TimeRange{Seconds{1s}, Seconds{2s}});
    auto range = result[0][0].As<BoundedTimeRange>();
    EXPECT_EQ(Seconds{1s}, utils::UnderlyingValue(range).GetLowerBound());
    /// [User timerange type usage]
  }
  UEXPECT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace

USERVER_NAMESPACE_END
