#include <deque>
#include <set>

#include <storages/postgres/tests/util_test.hpp>
#include <storages/postgres/typed_result_set.hpp>

namespace pg = storages::postgres;

namespace static_test {

using namespace pg::detail;

using MyTupleType = ::std::tuple<int, std::string, double>;

struct MyAggregateStruct {
  int int_member;
  std::string string_member;
  double double_member;
};

static_assert(boost::pfr::tuple_size_v<MyAggregateStruct> == 3, "");

class MyIntrusiveClass {
  int int_member_;
  std::string string_member_;
  double double_member_;

 public:
  auto Introspect() {
    return std::tie(int_member_, string_member_, double_member_);
  }

  int GetInt() const { return int_member_; }
  const std::string& GetString() const { return string_member_; }
  double GetDouble() const { return double_member_; }
};

struct MyPolymorphicBase {
  virtual ~MyPolymorphicBase() {}
};

struct MyPolymorphicDerived : MyPolymorphicBase {
  void Introspect() {}
};

class MyPolymorphicInrospected : public MyPolymorphicBase {
  int int_member_;
  std::string string_member_;
  double double_member_;

 public:
  auto Introspect() {
    return std::tie(int_member_, string_member_, double_member_);
  }
};

static_assert(kRowTag<MyTupleType> == RowTagType::kTuple, "");
static_assert(kRowTag<MyAggregateStruct> == RowTagType::kAggregate, "");
static_assert(kRowTag<MyIntrusiveClass> == RowTagType::kIntrusiveIntrospection,
              "");
static_assert(kRowTag<MyPolymorphicBase> == RowTagType::kInvalid, "");
static_assert(kRowTag<MyPolymorphicDerived> == RowTagType::kInvalid, "");
static_assert(kRowTag<MyPolymorphicInrospected> ==
                  RowTagType::kIntrusiveIntrospection,
              "");

static_assert(IsTuple<RowToUserData<MyTupleType>::ValueType>::value, "");
static_assert(IsTuple<RowToUserData<MyAggregateStruct>::TupleType>::value, "");
static_assert(IsTuple<RowToUserData<MyIntrusiveClass>::TupleType>::value, "");

}  // namespace static_test

namespace {

POSTGRE_TEST_P(TypedResult) {
  using MyTuple = static_test::MyTupleType;
  using MyStruct = static_test::MyAggregateStruct;
  using MyClass = static_test::MyIntrusiveClass;

  using MyTuples = std::vector<MyTuple>;
  using MyStructs = std::list<MyStruct>;
  using MyClasses = std::deque<MyClass>;

  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};

  EXPECT_NO_THROW(res = conn->Execute("select $1, $2, $3", 42, "foobar", 3.14));
  ASSERT_FALSE(res.IsEmpty());

  auto tuples_res = res.As<MyTuple>();
  auto t = tuples_res[0];
  EXPECT_EQ(42, std::get<0>(t));
  EXPECT_EQ("foobar", std::get<1>(t));
  EXPECT_EQ(3.14, std::get<2>(t));

  auto struct_res = res.As<MyStruct>();
  auto s = struct_res[0];
  EXPECT_EQ(42, s.int_member);
  EXPECT_EQ("foobar", s.string_member);
  EXPECT_EQ(3.14, s.double_member);

  auto class_res = res.As<MyClass>();
  auto c = class_res[0];
  EXPECT_EQ(42, c.GetInt());
  EXPECT_EQ("foobar", c.GetString());
  EXPECT_EQ(3.14, c.GetDouble());

  auto tuples = res.AsContainer<MyTuples>();
  EXPECT_EQ(res.Size(), tuples.size());
  auto structs = res.AsContainer<MyStructs>();
  EXPECT_EQ(res.Size(), structs.size());
  auto classes = res.AsContainer<MyClasses>();
  EXPECT_EQ(res.Size(), classes.size());

  auto tuple_set = res.AsContainer<std::set<MyTuple>>();
  EXPECT_EQ(res.Size(), tuple_set.size());
}

}  // namespace
