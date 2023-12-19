#include <deque>
#include <list>
#include <set>

#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/typed_result_set.hpp>

#include <userver/storages/postgres/io/boost_multiprecision.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace static_test {

namespace tt = pg::io::traits;

using MyTupleType = ::std::tuple<int, std::string, double>;

struct MyAggregateStruct {
  int int_member;
  std::string string_member;
  double double_member;
};

static_assert(boost::pfr::tuple_size_v<MyAggregateStruct> == 3);

struct MyStructWithOptional {
  std::optional<int> int_member;
  std::optional<std::string> string_member;
  std::optional<double> double_member;
};

static_assert(boost::pfr::tuple_size_v<MyStructWithOptional> == 3);

class MyIntrusiveClass {
  int int_member_ = 0;
  std::string string_member_{};
  double double_member_ = 0;

 public:
  auto Introspect() {
    return std::tie(int_member_, string_member_, double_member_);
  }

  int GetInt() const { return int_member_; }
  const std::string& GetString() const { return string_member_; }
  double GetDouble() const { return double_member_; }
};

struct MyPolymorphicBase {
  virtual ~MyPolymorphicBase() = default;
};

struct MyPolymorphicDerived : MyPolymorphicBase {};

class MyPolymorphicInrospected : public MyPolymorphicBase {
  int int_member_;
  std::string string_member_;
  double double_member_;

 public:
  auto Introspect() {
    return std::tie(int_member_, string_member_, double_member_);
  }
};

static_assert(tt::kRowCategory<std::string> == tt::RowCategoryType::kNonRow);
static_assert(tt::kRowCategory<std::vector<std::string>> ==
              tt::RowCategoryType::kNonRow);
static_assert(tt::kRowCategory<pg::MultiPrecision<50>> ==
              tt::RowCategoryType::kNonRow);

static_assert(tt::kRowCategory<MyTupleType> == tt::RowCategoryType::kTuple);
static_assert(tt::kRowCategory<MyAggregateStruct> ==
              tt::RowCategoryType::kAggregate);
static_assert(tt::kRowCategory<MyIntrusiveClass> ==
              tt::RowCategoryType::kIntrusiveIntrospection);
static_assert(tt::kRowCategory<MyPolymorphicBase> ==
              tt::RowCategoryType::kNonRow);
static_assert(tt::kRowCategory<MyPolymorphicDerived> ==
              tt::RowCategoryType::kNonRow);
static_assert(tt::kRowCategory<MyPolymorphicInrospected> ==
              tt::RowCategoryType::kIntrusiveIntrospection);

}  // namespace static_test

namespace {

UTEST_P(PostgreConnection, TypedResult) {
  using MyTuple = static_test::MyTupleType;
  using MyStruct = static_test::MyAggregateStruct;
  using MyClass = static_test::MyIntrusiveClass;

  using MyTuples = std::vector<MyTuple>;
  using MyStructs = std::list<MyStruct>;
  using MyClasses = std::deque<MyClass>;

  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1, $2, $3", 42, "foobar", 3.14));
  ASSERT_FALSE(res.IsEmpty());

  UEXPECT_THROW(res.AsSetOf<int>(), pg::NonSingleColumnResultSet);

  UEXPECT_THROW(res.AsSetOf<MyTuple>(), pg::NonSingleColumnResultSet);
  UEXPECT_THROW(res.AsSetOf<MyStruct>(), pg::NonSingleColumnResultSet);
  UEXPECT_THROW(res.AsSetOf<MyClass>(), pg::NonSingleColumnResultSet);

  auto tuples_res = res.AsSetOf<MyTuple>(pg::kRowTag);
  auto t = tuples_res[0];
  EXPECT_EQ(42, std::get<0>(t));
  EXPECT_EQ("foobar", std::get<1>(t));
  EXPECT_EQ(3.14, std::get<2>(t));

  auto struct_res = res.AsSetOf<MyStruct>(pg::kRowTag);
  auto s = struct_res[0];
  EXPECT_EQ(42, s.int_member);
  EXPECT_EQ("foobar", s.string_member);
  EXPECT_EQ(3.14, s.double_member);

  auto class_res = res.AsSetOf<MyClass>(pg::kRowTag);
  auto c = class_res[0];
  EXPECT_EQ(42, c.GetInt());
  EXPECT_EQ("foobar", c.GetString());
  EXPECT_EQ(3.14, c.GetDouble());

  auto tuples = res.AsContainer<MyTuples>(pg::kRowTag);
  EXPECT_EQ(res.Size(), tuples.size());
  auto structs = res.AsContainer<MyStructs>(pg::kRowTag);
  EXPECT_EQ(res.Size(), structs.size());
  auto classes = res.AsContainer<MyClasses>(pg::kRowTag);
  EXPECT_EQ(res.Size(), classes.size());

  auto tuple_set = res.AsContainer<std::set<MyTuple>>(pg::kRowTag);
  EXPECT_EQ(res.Size(), tuple_set.size());

  UEXPECT_NO_THROW(res.AsSingleRow<MyStruct>(pg::kRowTag));
  UEXPECT_NO_THROW(res.AsSingleRow<MyClass>(pg::kRowTag));
  UEXPECT_NO_THROW(res.AsSingleRow<MyTuple>(pg::kRowTag));
}

UTEST_P(PostgreConnection, TypedResultIterators) {
  using MyTuple = static_test::MyTupleType;

  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1, $2, $3", 42, "foobar", 3.14));
  ASSERT_EQ(1, res.Size());

  auto tuples_res = res.AsSetOf<MyTuple>(pg::kRowTag);
  auto forward_it = tuples_res.cbegin();
  auto reverse_it = tuples_res.crbegin();
  EXPECT_NE(forward_it, tuples_res.cend());
  EXPECT_NE(reverse_it, tuples_res.crend());
  EXPECT_EQ(std::get<0>(*forward_it), 42);
  EXPECT_EQ(std::get<1>(*forward_it), "foobar");
  EXPECT_EQ(std::get<2>(*forward_it), 3.14);
  EXPECT_EQ(*forward_it, *reverse_it);
  EXPECT_EQ(++forward_it, tuples_res.cend());
  EXPECT_EQ(++reverse_it, tuples_res.crend());
}

UTEST_P(PostgreConnection, OptionalFields) {
  using MyStruct = static_test::MyStructWithOptional;

  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  UEXPECT_NO_THROW(res = GetConn()->Execute("select 1, 'aa', null"));
  UEXPECT_NO_THROW(res.AsSingleRow<MyStruct>(pg::kRowTag));
}

UTEST_P(PostgreConnection, EmptyTypedResult) {
  using MyTuple = static_test::MyTupleType;
  using MyStruct = static_test::MyAggregateStruct;
  using MyClass = static_test::MyIntrusiveClass;

  CheckConnection(GetConn());
  auto empty_res =
      GetConn()->Execute("select $1, $2, $3 limit 0", 42, "foobar", 3.14);
  UEXPECT_THROW(empty_res.AsSingleRow<MyStruct>(), pg::NonSingleRowResultSet);
  UEXPECT_THROW(empty_res.AsSingleRow<MyClass>(), pg::NonSingleRowResultSet);
  UEXPECT_THROW(empty_res.AsSingleRow<MyTuple>(), pg::NonSingleRowResultSet);

  EXPECT_EQ(empty_res.begin(), empty_res.end());
  EXPECT_EQ(empty_res.cbegin(), empty_res.cend());
  EXPECT_EQ(empty_res.rbegin(), empty_res.rend());
  EXPECT_EQ(empty_res.crbegin(), empty_res.crend());
}

UTEST_P(PostgreConnection, TypedResultOobAccess) {
  using MyTuple = static_test::MyTupleType;
  using MyStruct = static_test::MyAggregateStruct;
  using MyClass = static_test::MyIntrusiveClass;

  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1, $2, $3", 42, "foobar", 3.14));

  auto tuples_res = res.AsSetOf<MyTuple>(pg::kRowTag);
  ASSERT_EQ(1, tuples_res.Size());
  UEXPECT_NO_THROW(tuples_res[0]);
  UEXPECT_THROW(tuples_res[1], pg::RowIndexOutOfBounds);

  auto struct_res = res.AsSetOf<MyStruct>(pg::kRowTag);
  ASSERT_EQ(1, struct_res.Size());
  UEXPECT_NO_THROW(struct_res[0]);
  UEXPECT_THROW(struct_res[1], pg::RowIndexOutOfBounds);

  auto class_res = res.AsSetOf<MyClass>(pg::kRowTag);
  ASSERT_EQ(1, class_res.Size());
  UEXPECT_NO_THROW(class_res[0]);
  UEXPECT_THROW(class_res[1], pg::RowIndexOutOfBounds);
}

UTEST_P(PostgreConnection, TypedResultTupleSample) {
  CheckConnection(GetConn());

  /// [RowTagSippet]
  const auto res = GetConn()->Execute("select $1, $2", 42, "foobar");

  using TupleType = std::tuple<int, std::string>;
  auto tuple = res.AsSingleRow<TupleType>(storages::postgres::kRowTag);
  EXPECT_EQ(std::get<0>(tuple), 42);
  EXPECT_EQ(std::get<1>(tuple), "foobar");
  /// [RowTagSippet]
}

}  // namespace

USERVER_NAMESPACE_END
