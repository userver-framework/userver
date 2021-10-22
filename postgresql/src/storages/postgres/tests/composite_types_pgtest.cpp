#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/composite_types.hpp>

USERVER_NAMESPACE_BEGIN

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

const std::string kCreateDomain = R"~(
create domain __pgtest.positive as integer check(value > 0)
)~";

const std::string kCreateCompositeWithDomain = R"~(
create type __pgtest.with_domain as (
    v __pgtest.positive
))~";

const std::string kDropSomeFields = R"~(
alter type __pgtest.foobar
  drop attribute i,
  drop attribute d,
  drop attribute v
)~";

const std::string kCreateCompositeWithArray = R"~(
create type __pgtest.with_array as (
    s integer[]
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
    return i == rhs.i && s == rhs.s && d == rhs.d && a == rhs.a && v == rhs.v;
  }
};

struct FooBarWithOptionalFields {
  std::optional<int> i;
  std::optional<std::string> s;
  std::optional<double> d;
  std::vector<int> a;
  std::vector<std::string> v;

  bool operator==(const FooBarWithOptionalFields& rhs) const {
    return i == rhs.i && s == rhs.s && d == rhs.d && a == rhs.a && v == rhs.v;
  }
};

using FooBarOpt = std::optional<FooBar>;

class FooClass {
  int i;
  std::string s;
  double d;
  std::vector<int> a;
  std::vector<std::string> v;

 public:
  FooClass() = default;
  explicit FooClass(int x) : i(x), s(std::to_string(x)), d(x), a{i}, v{s} {}

  // Only non-const version of Introspect() is used by the uPostgres driver
  auto Introspect() { return std::tie(i, s, d, a, v); }

  auto GetI() const { return i; }
  auto GetS() const { return s; }
  auto GetD() const { return d; }
  auto GetA() const { return a; }
  auto GetV() const { return v; }
};

using FooTuple = std::tuple<int, std::string, double, std::vector<int>,
                            std::vector<std::string>>;

struct BunchOfFoo {
  std::vector<FooBar> foobars;

  bool operator==(const BunchOfFoo& rhs) const {
    return foobars == rhs.foobars;
  }
};

}  // namespace pgtest
/*! [User type declaration] */

namespace pgtest {

// These declarations are separate from the others as they shouldn't get into
// the code snippet for documentation.
struct NoUseInWrite {
  int i;
  std::string s;
  double d;
  std::vector<int> a;
  std::vector<std::string> v;
};

struct NoUserMapping {
  int i;
  std::string s;
  double d;
  std::vector<int> a;
  std::vector<std::string> v;

  bool operator==(const NoUserMapping& rhs) const {
    return i == rhs.i && s == rhs.s && d == rhs.d && a == rhs.a && v == rhs.v;
  }
  bool operator==(const FooBar& rhs) const {
    return i == rhs.i && s == rhs.s && d == rhs.d && a == rhs.a && v == rhs.v;
  }
};

struct NoUserMappingBunch {
  std::vector<NoUserMapping> elements;

  bool operator==(const NoUserMappingBunch& rhs) const {
    return elements == rhs.elements;
  }
  bool operator==(const BunchOfFoo& rhs) const {
    if (elements.size() != rhs.foobars.size()) return false;
    for (size_t i = 0; i < elements.size(); ++i) {
      if (!(elements[i] == rhs.foobars[i])) return false;
    }
    return true;
  }
};

struct WithDomain {
  int v;
};

struct FooBarWithSomeFieldsDropped {
  std::string s;
  std::vector<int> a;

  bool operator==(const FooBarWithSomeFieldsDropped& rhs) const {
    return s == rhs.s && a == rhs.a;
  }
};

struct WithUnorderedSet {
  std::unordered_set<int> s;

  bool operator==(const WithUnorderedSet& rhs) const { return s == rhs.s; }
};

}  // namespace pgtest

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

// These mappings are separate from the others as they shouldn't get into the
// code snippet for documentation.
template <>
struct CppToUserPg<pgtest::NoUseInWrite> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

template <>
struct CppToUserPg<pgtest::FooBarWithOptionalFields> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

template <>
struct CppToUserPg<pgtest::WithDomain> {
  static constexpr DBTypeName postgres_name = "__pgtest.with_domain";
};

template <>
struct CppToUserPg<pgtest::FooBarWithSomeFieldsDropped> {
  static constexpr DBTypeName postgres_name = kCompositeName;
};

template <>
struct CppToUserPg<pgtest::WithUnorderedSet> {
  static constexpr DBTypeName postgres_name = "__pgtest.with_array";
};

}  // namespace storages::postgres::io

namespace static_test {

static_assert(io::traits::TupleHasParsers<pgtest::FooTuple>::value);
static_assert(tt::detail::CompositeHasParsers<pgtest::FooTuple>::value);
static_assert(tt::detail::CompositeHasParsers<pgtest::FooBar>::value);
static_assert(
    tt::detail::CompositeHasParsers<pgtest::FooBarWithOptionalFields>::value);
static_assert(tt::detail::CompositeHasParsers<pgtest::FooClass>::value);
static_assert(tt::detail::CompositeHasParsers<pgtest::NoUseInWrite>::value);
static_assert(tt::detail::CompositeHasParsers<pgtest::NoUserMapping>::value);
static_assert(tt::detail::CompositeHasParsers<pgtest::WithUnorderedSet>::value);

static_assert(!tt::detail::CompositeHasParsers<int>::value);

static_assert(io::traits::TupleHasFormatters<pgtest::FooTuple>::value);
static_assert(tt::detail::CompositeHasFormatters<pgtest::FooTuple>::value);
static_assert(tt::detail::CompositeHasFormatters<pgtest::FooBar>::value);
static_assert(tt::detail::CompositeHasFormatters<pgtest::FooClass>::value);

static_assert(!tt::detail::CompositeHasParsers<int>::value);

static_assert(tt::kHasParser<pgtest::BunchOfFoo>);
static_assert(tt::kHasFormatter<pgtest::BunchOfFoo>);

static_assert(tt::kHasParser<pgtest::NoUserMapping>);
static_assert(!tt::kHasFormatter<pgtest::NoUserMapping>);

static_assert(tt::kTypeBufferCategory<pgtest::FooTuple> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::FooBar> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::FooBarWithOptionalFields> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::FooClass> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::BunchOfFoo> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::NoUseInWrite> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::NoUserMapping> ==
              io::BufferCategory::kCompositeBuffer);
static_assert(tt::kTypeBufferCategory<pgtest::WithUnorderedSet> ==
              io::BufferCategory::kCompositeBuffer);

}  // namespace static_test

namespace {

UTEST_F(PostgreConnection, CompositeTypeRoundtrip) {
  CheckConnection(conn);
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
  const std::vector<int> expected_int_vector{-1, 0, 1};
  const std::vector<std::string> expected_str_vector{"a", "b", "c"};

  ASSERT_FALSE(res.IsEmpty());

  pgtest::FooBar fb;
  EXPECT_NO_THROW(res[0].To(fb));
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ(42, fb.i);
  EXPECT_EQ("foobar", fb.s);
  EXPECT_EQ(3.14, fb.d);
  EXPECT_EQ(expected_int_vector, fb.a);
  EXPECT_EQ(expected_str_vector, fb.v);

  pgtest::FooTuple ft;
  EXPECT_NO_THROW(res[0].To(ft));
  EXPECT_EQ(42, std::get<0>(ft));
  EXPECT_EQ("foobar", std::get<1>(ft));
  EXPECT_EQ(3.14, std::get<2>(ft));
  EXPECT_EQ(expected_int_vector, std::get<3>(ft));
  EXPECT_EQ(expected_str_vector, std::get<4>(ft));

  pgtest::FooClass fc;
  EXPECT_NO_THROW(res[0].To(fc));
  EXPECT_EQ(42, fc.GetI());
  EXPECT_EQ("foobar", fc.GetS());
  EXPECT_EQ(3.14, fc.GetD());
  EXPECT_EQ(expected_int_vector, fc.GetA());
  EXPECT_EQ(expected_str_vector, fc.GetV());

  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fb));
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", ft));
  EXPECT_NO_THROW(res = conn->Execute("select $1 as foo", fc));

  using FooVector = std::vector<pgtest::FooBar>;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1 as array_of_foo", FooVector{fb, fb, fb}));

  ASSERT_FALSE(res.IsEmpty());
  EXPECT_THROW(res[0][0].As<pgtest::FooBar>(), pg::InvalidParserCategory);
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ((FooVector{fb, fb, fb}), res[0].As<FooVector>());

  pgtest::BunchOfFoo bf{{fb, fb, fb}};
  EXPECT_NO_THROW(res = conn->Execute("select $1 as bunch", bf));
  ASSERT_FALSE(res.IsEmpty());
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
      << "A type that is not used for writing query parameter buffers must be "
         "available for reading";

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

UTEST_F(PostgreConnection, CompositeWithOptionalFieldsRoundtrip) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  // Auto reload doesn't work for outgoing types
  ASSERT_NO_THROW(conn->ReloadUserTypes());

  pgtest::FooBarWithOptionalFields fb;
  EXPECT_NO_THROW(res = conn->Execute("select $1", fb));
  EXPECT_EQ(fb,
            res.AsSingleRow<pgtest::FooBarWithOptionalFields>(pg::kFieldTag));

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

UTEST_F(PostgreConnection, OptionalCompositeTypeRoundtrip) {
  CheckConnection(conn);
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

UTEST_F(PostgreConnection, CompositeTypeWithDomainRoundtrip) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  ASSERT_NO_THROW(conn->Execute(kCreateDomain)) << "Create domain";
  ASSERT_NO_THROW(conn->Execute(kCreateCompositeWithDomain))
      << "Create composite";
  // Auto reload doesn't work for outgoing types
  ASSERT_NO_THROW(conn->ReloadUserTypes());

  EXPECT_NO_THROW(res = conn->Execute("select $1::__pgtest.with_domain",
                                      pgtest::WithDomain{1}));
  EXPECT_NO_THROW(res.AsSingleRow<pgtest::WithDomain>(pg::kFieldTag));
  auto v = res.AsSingleRow<pgtest::WithDomain>(pg::kFieldTag);
  EXPECT_EQ(1, v.v);
}

UTEST_F(PostgreConnection, CompositeTypeRoundtripAsRecord) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  EXPECT_NO_THROW(conn->Execute(kCreateCompositeOfComposites))
      << "Successfully create composite of composites";

  EXPECT_NO_THROW(
      res = conn->Execute(
          "SELECT fb.* FROM (SELECT ROW(42, 'foobar', 3.14, ARRAY[-1, 0, 1], "
          "ARRAY['a', 'b', 'c'])::__pgtest.foobar) fb"));
  const std::vector<int> expected_int_vector{-1, 0, 1};
  const std::vector<std::string> expected_str_vector{"a", "b", "c"};

  ASSERT_FALSE(res.IsEmpty());

  pgtest::FooBar fb;
  res[0].To(fb);
  EXPECT_NO_THROW(res[0].To(fb));
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ(42, fb.i);
  EXPECT_EQ("foobar", fb.s);
  EXPECT_EQ(3.14, fb.d);
  EXPECT_EQ(expected_int_vector, fb.a);
  EXPECT_EQ(expected_str_vector, fb.v);

  pgtest::FooTuple ft;
  EXPECT_NO_THROW(res[0].To(ft));
  EXPECT_EQ(42, std::get<0>(ft));
  EXPECT_EQ("foobar", std::get<1>(ft));
  EXPECT_EQ(3.14, std::get<2>(ft));
  EXPECT_EQ(expected_int_vector, std::get<3>(ft));
  EXPECT_EQ(expected_str_vector, std::get<4>(ft));

  pgtest::FooClass fc;
  EXPECT_NO_THROW(res[0].To(fc));
  EXPECT_EQ(42, fc.GetI());
  EXPECT_EQ("foobar", fc.GetS());
  EXPECT_EQ(3.14, fc.GetD());
  EXPECT_EQ(expected_int_vector, fc.GetA());
  EXPECT_EQ(expected_str_vector, fc.GetV());

  pgtest::NoUserMapping nm;
  EXPECT_NO_THROW(res[0].To(nm));
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ(42, nm.i);
  EXPECT_EQ("foobar", nm.s);
  EXPECT_EQ(3.14, nm.d);
  EXPECT_EQ(expected_int_vector, nm.a);
  EXPECT_EQ(expected_str_vector, nm.v);

  EXPECT_NO_THROW(res = conn->Execute("SELECT ROW($1.*) AS record", fb));
  EXPECT_NO_THROW(res = conn->Execute("SELECT ROW($1.*) AS record", ft));
  EXPECT_NO_THROW(res = conn->Execute("SELECT ROW($1.*) AS record", fc));

  using FooVector = std::vector<pgtest::FooBar>;
  EXPECT_NO_THROW(res = conn->Execute("SELECT $1::record[] AS array_of_records",
                                      FooVector{fb, fb, fb}));

  ASSERT_FALSE(res.IsEmpty());
  EXPECT_THROW(res[0][0].As<pgtest::FooBar>(), pg::InvalidParserCategory);
  EXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
  EXPECT_EQ((FooVector{fb, fb, fb}), res[0].As<FooVector>());

  pgtest::BunchOfFoo bf{{fb, fb, fb}};
  EXPECT_NO_THROW(res =
                      conn->Execute("SELECT ROW($1.f::record[]) AS bunch", bf));
  ASSERT_FALSE(res.IsEmpty());

  pgtest::BunchOfFoo bf1;
  EXPECT_NO_THROW(res[0].To(bf1));
  EXPECT_EQ(bf, bf1);
  EXPECT_EQ(bf, res[0].As<pgtest::BunchOfFoo>());

  pgtest::NoUserMappingBunch bnm;
  EXPECT_NO_THROW(res[0].To(bnm));
  EXPECT_EQ(bnm, bf);
  EXPECT_EQ(bnm, res[0].As<pgtest::NoUserMappingBunch>());

  // Unwrapping composite structure to a row
  EXPECT_NO_THROW(res = conn->Execute("select $1.f::record[]", bf));
  ASSERT_FALSE(res.IsEmpty());
  EXPECT_NO_THROW(res[0].To(bf1, pg::kRowTag));
  EXPECT_EQ(bf, bf1);
  EXPECT_EQ(bf, res[0].As<pgtest::BunchOfFoo>(pg::kRowTag));

  EXPECT_ANY_THROW(res[0][0].To(bf1));

  // Using a mapped type only for reading
  EXPECT_NO_THROW(res = conn->Execute("SELECT ROW($1.*) AS record", fb));
  EXPECT_NO_THROW(res.AsContainer<std::vector<pgtest::NoUseInWrite>>())
      << "A type that is not used for writing query parameter buffers must be "
         "available for reading";

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

UTEST_F(PostgreConnection, OptionalCompositeTypeRoundtripAsRecord) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";

  EXPECT_NO_THROW(
      res = conn->Execute(
          "SELECT fb.* FROM (SELECT ROW(42, 'foobar', 3.14, ARRAY[-1, 0, 1], "
          "ARRAY['a', 'b', 'c'])::__pgtest.foobar) fb"));
  {
    auto fo = res.Front().As<pgtest::FooBarOpt>();
    EXPECT_TRUE(!!fo) << "Non-empty optional result expected";
  }

  EXPECT_NO_THROW(res = conn->Execute("select null::record"));
  {
    auto fo = res.Front().As<pgtest::FooBarOpt>();
    EXPECT_TRUE(!fo) << "Empty optional result expected";
  }

  EXPECT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
}

// Please never use this in your code, this is only to check type loaders
UTEST_F(PostgreConnection, VariableRecordTypes) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(
      res = conn->Execute("WITH test AS (SELECT unnest(ARRAY[1, 2]) a)"
                          "SELECT CASE WHEN a = 1 THEN ROW(42)"
                          "WHEN a = 2 THEN ROW('str'::text) "
                          "END FROM test"));
  ASSERT_EQ(2, res.Size());

  EXPECT_EQ(42, std::get<0>(res[0].As<std::tuple<int>>()));
  EXPECT_EQ("str", std::get<0>(res[1].As<std::tuple<std::string>>()));
}

// This is not exactly allowed as well, we just don't want to crash on legacy
UTEST_F(PostgreConnection, CompositeDroppedFields) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateACompositeType))
      << "Successfully create a composite type";
  EXPECT_NO_THROW(conn->Execute(kDropSomeFields))
      << "Drop some composite type fields";

  pg::ResultSet res{nullptr};
  // The datatypes are expected to be automatically reloaded
  EXPECT_NO_THROW(
      res = conn->Execute(
          "select ROW('foobar', ARRAY[-1, 0, 1])::__pgtest.foobar"));
  const std::vector<int> expected_int_vector{-1, 0, 1};

  ASSERT_FALSE(res.IsEmpty());

  pgtest::FooBarWithSomeFieldsDropped fb;
  EXPECT_NO_THROW(res[0].To(fb));
  EXPECT_EQ("foobar", fb.s);
  EXPECT_EQ(expected_int_vector, fb.a);

  EXPECT_NO_THROW(res = conn->Execute("select $1", fb));
  EXPECT_EQ(res.AsSingleRow<pgtest::FooBarWithSomeFieldsDropped>(), fb);
}

UTEST_F(PostgreConnection, CompositeUnorderedSet) {
  CheckConnection(conn);
  ASSERT_FALSE(conn->IsReadOnly()) << "Expect a read-write connection";

  ASSERT_NO_THROW(conn->Execute(kDropTestSchema)) << "Drop schema";
  ASSERT_NO_THROW(conn->Execute(kCreateTestSchema)) << "Create schema";

  EXPECT_NO_THROW(conn->Execute(kCreateCompositeWithArray))
      << "Successfully create a composite type";

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(
      res = conn->Execute("select ROW(ARRAY[-1, 0, 1])::__pgtest.with_array"));
  const std::unordered_set<int> expected_int_set{-1, 0, 1};

  ASSERT_FALSE(res.IsEmpty());

  pgtest::WithUnorderedSet usc;
  EXPECT_NO_THROW(res[0].To(usc));
  EXPECT_EQ(expected_int_set, usc.s);

  EXPECT_NO_THROW(res = conn->Execute("select $1", usc));
  EXPECT_EQ(res.AsSingleRow<pgtest::WithUnorderedSet>(), usc);

  EXPECT_NO_THROW(res = conn->Execute("select $1.*", usc));
  EXPECT_EQ(res.AsSingleRow<pgtest::WithUnorderedSet>(pg::kRowTag), usc);
}

}  // namespace

USERVER_NAMESPACE_END
