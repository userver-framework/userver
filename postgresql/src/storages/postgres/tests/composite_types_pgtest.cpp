#include <storages/postgres/io/composite_types.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

constexpr const char* const kSchemaName = "__pgtest";
const std::string kCreateTestSchema = "create schema if not exists __pgtest";
const std::string kDropTestSchema = "drop schema if exists __pgtest cascade";

constexpr pg::DBTypeName kCompositeName{kSchemaName, "foobar"};
const std::string kCreateACompositeType = R"~(
create type __pgtest.foobar as (
  i integer,
  s text,
  d double precision,
  a integer[],
  v varchar[]
))~";

const std::string kCreateCompositeOfComposites = R"~(
create type __pgtest.foobars as (
  f __pgtest.foobar[]
))~";

}  // namespace

/*! [User type declaration] */
namespace pgtest {

struct FooBar {
  int i;
  std::string s;
  double d;
  std::vector<int> a;
  std::vector<std::string> v;

  bool operator==(const FooBar& rhs) const {
    return i == rhs.i && s == rhs.s && d == rhs.d && a == rhs.a;
  }
};

using FooBarOpt = boost::optional<FooBar>;

class FooClass {
  int i;
  std::string s;
  double d;
  std::vector<int> a;
  std::vector<std::string> v;

 public:
  auto Introspect() { return std::tie(i, s, d, a, v); }

  auto GetI() const { return i; }
  auto GetS() const { return s; }
  auto GetD() const { return d; }
  auto GetA() const { return a; }
};

using FooTuple = std::tuple<int, std::string, double, std::vector<int>,
                            std::vector<std::string>>;

struct BunchOfFoo {
  std::vector<FooBar> foobars;

  bool operator==(const BunchOfFoo& rhs) const {
    return foobars == rhs.foobars;
  }
};

struct NoUseInWrite {
  int i;
  std::string s;
  double d;
  std::vector<int> a;
  std::vector<std::string> v;
};

}  // namespace pgtest
/*! [User type declaration] */

/*! [User type mapping] */
namespace storages::postgres::io {

// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pgtest::FooBar> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pgtest::FooClass> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

// This specialisation MUST go to the header together with the mapped type
template <>
struct CppToUserPg<pgtest::FooTuple> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

template <>
struct CppToUserPg<pgtest::BunchOfFoo> {
  static constexpr DBTypeName postgres_name = "__pgtest.foobars";
};

}  // namespace storages::postgres::io
/*! [User type mapping] */

namespace storages::postgres::io {

// This mapping is separate from the others as it shouldn't get to the code
// snippet for generating documentation.
template <>
struct CppToUserPg<pgtest::NoUseInWrite> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

}  // namespace storages::postgres::io

namespace static_test {

static_assert((io::traits::TupleHasParsers<
                  pgtest::FooTuple, io::DataFormat::kBinaryDataFormat>::value),
              "");
static_assert((tt::detail::CompositeHasParsers<
                  pgtest::FooTuple, io::DataFormat::kBinaryDataFormat>::value),
              "");
static_assert((tt::detail::CompositeHasParsers<
                  pgtest::FooBar, io::DataFormat::kBinaryDataFormat>::value),
              "");
static_assert((tt::detail::CompositeHasParsers<
                  pgtest::FooClass, io::DataFormat::kBinaryDataFormat>::value),
              "");

static_assert((!tt::detail::CompositeHasParsers<
                  int, io::DataFormat::kBinaryDataFormat>::value),
              "");

static_assert(tt::kHasAnyParser<pgtest::BunchOfFoo>, "");
static_assert(tt::kHasAnyFormatter<pgtest::BunchOfFoo>, "");

static_assert(tt::kTypeBufferCategory<pgtest::FooTuple> ==
                  io::BufferCategory::kCompositeBuffer,
              "");
static_assert(tt::kTypeBufferCategory<pgtest::FooBar> ==
                  io::BufferCategory::kCompositeBuffer,
              "");
static_assert(tt::kTypeBufferCategory<pgtest::FooClass> ==
                  io::BufferCategory::kCompositeBuffer,
              "");
static_assert(tt::kTypeBufferCategory<pgtest::BunchOfFoo> ==
                  io::BufferCategory::kCompositeBuffer,
              "");

}  // namespace static_test

namespace {

const pg::UserTypes types;

pg::io::TypeBufferCategory GetTestTypeCategories() {
  pg::io::TypeBufferCategory result;
  using Oids = pg::io::PredefinedOids;
  for (auto p_oid : {Oids::kInt2, Oids::kInt2Array, Oids::kInt4,
                     Oids::kInt4Array, Oids::kInt8, Oids::kInt8Array}) {
    auto oid = static_cast<pg::Oid>(p_oid);
    result.insert(std::make_pair(oid, types.GetBufferCategory(oid)));
  }
  return result;
}

const pg::io::TypeBufferCategory categories = GetTestTypeCategories();

POSTGRE_TEST_P(CompositeTypeRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  EXPECT_NO_THROW(conn->Execute(kCreateCompositeOfComposites))
      << "Successfully create composite of composites";

  // The datatypes are expected to be automatically reloaded
  EXPECT_NO_THROW(
      res = conn->Execute("select ROW(42, 'foobar', 3.14, ARRAY[-1, 0, 1], "
                          "ARRAY['a', 'b', 'c'])::__pgtest.foobar"));
  std::vector<int> expected_vector{-1, 0, 1};

  ASSERT_FALSE(res.IsEmpty());
  ASSERT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());

  pgtest::FooBar fb;
  EXPECT_NO_THROW(res[0].To(fb));
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ(42, fb.i);
  EXPECT_EQ("foobar", fb.s);
  EXPECT_EQ(3.14, fb.d);
  EXPECT_EQ(expected_vector, fb.a);

  pgtest::FooTuple ft;
  EXPECT_NO_THROW(res[0].To(ft));
  EXPECT_EQ(42, std::get<0>(ft));
  EXPECT_EQ("foobar", std::get<1>(ft));
  EXPECT_EQ(3.14, std::get<2>(ft));
  EXPECT_EQ(expected_vector, std::get<3>(ft));

  pgtest::FooClass fc;
  EXPECT_NO_THROW(res[0].To(fc));
  EXPECT_EQ(42, fc.GetI());
  EXPECT_EQ("foobar", fc.GetS());
  EXPECT_EQ(3.14, fc.GetD());
  EXPECT_EQ(expected_vector, fc.GetA());

  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fb));
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", ft));
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fc));

  using FooVector = std::vector<pgtest::FooBar>;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1 as array_of_foo", FooVector{fb, fb, fb}));

  ASSERT_FALSE(res.IsEmpty());
  ASSERT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
  EXPECT_THROW(res[0][0].As<pgtest::FooBar>(), pg::InvalidParserCategory);
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ((FooVector{fb, fb, fb}), res[0].As<FooVector>());

  pgtest::BunchOfFoo bf{{fb, fb, fb}};
  res = conn->Execute("select $1 as bunch", bf);
  EXPECT_NO_THROW(res = conn->Execute("select $1 as bunch", bf));
  ASSERT_FALSE(res.IsEmpty());
  ASSERT_EQ(io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
  pgtest::BunchOfFoo bf1;
  EXPECT_NO_THROW(res[0].To(bf1));
  EXPECT_EQ(bf, bf1);
  EXPECT_EQ(bf, res[0].As<pgtest::BunchOfFoo>());

  // Unwrapping composite structure to a row
  EXPECT_NO_THROW(res = conn->Execute("select $1.*", bf));
  ASSERT_FALSE(res.IsEmpty());
  EXPECT_NO_THROW(res[0].To(bf1, pg::kRowTag));
  EXPECT_EQ(bf, bf1);
  EXPECT_EQ(bf, res[0].As<pgtest::BunchOfFoo>(pg::kRowTag));

  EXPECT_ANY_THROW(res[0][0].To(bf1));

  // Using a mapped type only for reading
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fb));
  EXPECT_NO_THROW(res.AsContainer<std::vector<pgtest::NoUseInWrite>>())
      << "A type that is not used for writing query parameter buffers must be"
         "available for reading";

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

POSTGRE_TEST_P(OptionalCompositeTypeRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";

  EXPECT_NO_THROW(
      res = conn->Execute("select ROW(42, 'foobar', 3.14, ARRAY[-1, 0, 1], "
                          "ARRAY['a', 'b', 'c'])::__pgtest.foobar"));
  {
    auto fo = res.Front().As<pgtest::FooBarOpt>();
    EXPECT_TRUE(!!fo) << "Non-empty optional result expected";
  }

  EXPECT_NO_THROW(res = conn->Execute("select null::__pgtest.foobar"));
  {
    auto fo = res.Front().As<pgtest::FooBarOpt>();
    EXPECT_TRUE(!fo) << "Empty optional result expected";
  }

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

}  // namespace
