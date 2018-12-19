#include <storages/postgres/io/composite_types.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

constexpr const char* const kSchemaName = "__pg_test";
const std::string kCreateTestSchema = "create schema if not exists __pg_test";
const std::string kDropTestSchema = "drop schema if exists __pg_test cascade";

constexpr pg::DBTypeName kCompositeName{kSchemaName, "foobar"};
const std::string kCreateACompositeType = R"~(
create type __pg_test.foobar as (
  i integer,
  s text,
  d double precision,
  a integer[]
))~";

}  // namespace

/*! [User type declaration] */
namespace pg_test {

struct FooBar {
  int i;
  std::string s;
  double d;
  std::vector<int> a;

  bool operator==(const FooBar& rhs) const {
    return i == rhs.i && s == rhs.s && d == rhs.d && a == rhs.a;
  }
};

class FooClass {
  int i;
  std::string s;
  double d;
  std::vector<int> a;

 public:
  auto Introspect() { return std::tie(i, s, d, a); }

  auto GetI() const { return i; }
  auto GetS() const { return s; }
  auto GetD() const { return d; }
  auto GetA() const { return a; }
};

using FooTuple = std::tuple<int, std::string, double, std::vector<int>>;

}  // namespace pg_test
/*! [User type declaration] */

/*! [User type mapping] */
namespace storages::postgres::io {

// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pg_test::FooBar> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pg_test::FooClass> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pg_test::FooTuple> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

}  // namespace storages::postgres::io
/*! [User type mapping] */

namespace static_test {

static_assert((io::traits::TupleHasParsers<
                  pg_test::FooTuple, io::DataFormat::kBinaryDataFormat>::value),
              "");
static_assert((tt::detail::CompositeHasParsers<
                  pg_test::FooTuple, io::DataFormat::kBinaryDataFormat>::value),
              "");
static_assert((tt::detail::CompositeHasParsers<
                  pg_test::FooBar, io::DataFormat::kBinaryDataFormat>::value),
              "");
static_assert((tt::detail::CompositeHasParsers<
                  pg_test::FooClass, io::DataFormat::kBinaryDataFormat>::value),
              "");

static_assert((!tt::detail::CompositeHasParsers<
                  int, io::DataFormat::kBinaryDataFormat>::value),
              "");
}  // namespace static_test

namespace {

POSTGRE_TEST_P(CompositeTypeRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";

  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  EXPECT_NO_THROW(conn->ReloadUserTypes()) << "Reload user types";

  EXPECT_NO_THROW(
      res = conn->Execute(
          "select ROW(42, 'foobar', 3.14, ARRAY[-1, 0, 1])::__pg_test.foobar"));
  std::vector<int> expected_vector{-1, 0, 1};

  ASSERT_TRUE(res);
  ASSERT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());

  pg_test::FooBar fb;
  EXPECT_NO_THROW(res[0].To(fb));
  EXPECT_EQ(42, fb.i);
  EXPECT_EQ("foobar", fb.s);
  EXPECT_EQ(3.14, fb.d);
  EXPECT_EQ(expected_vector, fb.a);

  pg_test::FooTuple ft;
  EXPECT_NO_THROW(res[0].To(ft));
  EXPECT_EQ(42, std::get<0>(ft));
  EXPECT_EQ("foobar", std::get<1>(ft));
  EXPECT_EQ(3.14, std::get<2>(ft));
  EXPECT_EQ(expected_vector, std::get<3>(ft));

  pg_test::FooClass fc;
  EXPECT_NO_THROW(res[0].To(fc));
  EXPECT_EQ(42, fc.GetI());
  EXPECT_EQ("foobar", fc.GetS());
  EXPECT_EQ(3.14, fc.GetD());
  EXPECT_EQ(expected_vector, fc.GetA());

  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fb));
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", ft));
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fc));

  using FooVector = std::vector<pg_test::FooBar>;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1 as array_of_foo", FooVector{fb, fb, fb}));

  ASSERT_TRUE(res);
  ASSERT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
  EXPECT_EQ((FooVector{fb, fb, fb}), res[0][0].As<FooVector>());

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace
