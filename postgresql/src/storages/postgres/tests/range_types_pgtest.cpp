#include <userver/storages/postgres/io/range_types.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

TEST(Postgre, RangeTypeEmpty) {
  pg::IntegerRange r;

  EXPECT_TRUE(r.Empty()) << "Default constructed range should be empty";
  EXPECT_FALSE(r.HasLowerBound()) << "Empty range doesn't have a lower bound";
  EXPECT_FALSE(r.HasUpperBound()) << "Empty range doesn't have an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Empty range doesn't include the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Empty range doesn't include the upper bound";

  EXPECT_EQ(pg::IntegerRange{}, pg::MakeRange(0, 0))
      << "Equal range ends with upper not included is an empty range";
  EXPECT_EQ(pg::IntegerRange{}, pg::MakeRange(0, 0, pg::RangeBound::kUpper))
      << "Equal range ends with lower not included is an empty range";
  EXPECT_NE(pg::IntegerRange{}, pg::MakeRange(0, 0, pg::RangeBound::kBoth))
      << "Equal range ends with both included is not an empty range";
}

TEST(Postgre, RangeTypeIncludeLower) {
  auto r = pg::MakeRange(0, 1);
  EXPECT_FALSE(r.Empty()) << "Range [0, 1) is not empty";
  EXPECT_TRUE(r.HasLowerBound()) << "Range [0, 1) has a lower bound";
  EXPECT_TRUE(r.HasUpperBound()) << "Range [0, 1) has an upper bound";
  EXPECT_TRUE(r.IsLowerBoundIncluded())
      << "Range [0, 1) includes the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Range [0, 1) doesn't include the upper bound";
}

TEST(Postgre, RangeTypeIncludeUpper) {
  auto r = pg::MakeRange(0, 1, pg::RangeBound::kUpper);
  EXPECT_FALSE(r.Empty()) << "Range (0, 1] is not empty";
  EXPECT_TRUE(r.HasLowerBound()) << "Range (0, 1] has a lower bound";
  EXPECT_TRUE(r.HasUpperBound()) << "Range (0, 1] has an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Range (0, 1] doesn't include the lower bound";
  EXPECT_TRUE(r.IsUpperBoundIncluded())
      << "Range (0, 1] includes the upper bound";
}

TEST(Postgre, RangeTypeIncludeBoth) {
  auto r = pg::MakeRange(0, 1, pg::RangeBound::kBoth);
  EXPECT_FALSE(r.Empty()) << "Range [0, 1] is not empty";
  EXPECT_TRUE(r.HasLowerBound()) << "Range [0, 1] has a lower bound";
  EXPECT_TRUE(r.HasUpperBound()) << "Range [0, 1] has an upper bound";
  EXPECT_TRUE(r.IsLowerBoundIncluded())
      << "Range [0, 1] includes the lower bound";
  EXPECT_TRUE(r.IsUpperBoundIncluded())
      << "Range [0, 1] includes the upper bound";
}

TEST(Postgre, RangeTypeIncludeNone) {
  auto r = pg::MakeRange(0, 1, pg::RangeBound::kNone);
  EXPECT_FALSE(r.Empty()) << "Range (0, 1) is not empty";
  EXPECT_TRUE(r.HasLowerBound()) << "Range (0, 1) has a lower bound";
  EXPECT_TRUE(r.HasUpperBound()) << "Range (0, 1) has an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Range (0, 1) doesn't include the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Range (0, 1) doesn't include the upper bound";
}

TEST(Postgre, RangeTypeUnboundedUpperIncludeLower) {
  auto r = pg::MakeRange(0, pg::kUnbounded);
  EXPECT_FALSE(r.Empty()) << "Range [0, ∞) is not empty";
  EXPECT_TRUE(r.HasLowerBound()) << "Range [0, ∞) has a lower bound";
  EXPECT_FALSE(r.HasUpperBound()) << "Range [0, ∞) doesn't have an upper bound";
  EXPECT_TRUE(r.IsLowerBoundIncluded())
      << "Range [0, ∞) includes the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Range [0, ∞) doesn't include the upper bound";
}

TEST(Postgre, RangeTypeUnboundedUpperExcludeLower) {
  auto r = pg::MakeRange(0, pg::kUnbounded, pg::RangeBound::kNone);
  EXPECT_FALSE(r.Empty()) << "Range (0, ∞) is not empty";
  EXPECT_TRUE(r.HasLowerBound()) << "Range (0, ∞) has a lower bound";
  EXPECT_FALSE(r.HasUpperBound()) << "Range (0, ∞) doesn't have an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Range (0, ∞) doesn't include the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Range (0, ∞) doesn't include the upper bound";
}

TEST(Postgre, RangeTypeUnboundedLowerIncludeUpper) {
  auto r = pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kUpper);
  EXPECT_FALSE(r.Empty()) << "Range (-∞, 0] is not empty";
  EXPECT_FALSE(r.HasLowerBound()) << "Range (-∞, 0] doesn't have a lower bound";
  EXPECT_TRUE(r.HasUpperBound()) << "Range (-∞, 0] has an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Range (-∞, 0] doesn't include the lower bound";
  EXPECT_TRUE(r.IsUpperBoundIncluded())
      << "Range (-∞, 0] includes the upper bound";
}

TEST(Postgre, RangeTypeUnboundedLowerExcludeUpper) {
  auto r = pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kNone);
  EXPECT_FALSE(r.Empty()) << "Range (-∞, 0) is not empty";
  EXPECT_FALSE(r.HasLowerBound()) << "Range (-∞, 0) doesn't have a lower bound";
  EXPECT_TRUE(r.HasUpperBound()) << "Range (-∞, 0) has an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Range (-∞, 0) doesn't include the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Range (-∞, 0) doesn't include the upper bound";
}

TEST(Postgre, RangeTypeUnboundedBoth) {
  pg::IntegerRange r{pg::kUnbounded, pg::kUnbounded};
  EXPECT_FALSE(r.Empty()) << "Range (-∞, ∞) is not empty";
  EXPECT_FALSE(r.HasLowerBound()) << "Range (-∞, ∞) doesn't have a lower bound";
  EXPECT_FALSE(r.HasUpperBound())
      << "Range (-∞, ∞) doesn't have an upper bound";
  EXPECT_FALSE(r.IsLowerBoundIncluded())
      << "Range (-∞, ∞) doesn't include the lower bound";
  EXPECT_FALSE(r.IsUpperBoundIncluded())
      << "Range (-∞, ∞) doesn't include the upper bound";
}

TEST(Postgre, RangeEquality) {
  EXPECT_EQ(pg::IntegerRange{}, pg::IntegerRange{});
  EXPECT_EQ((pg::IntegerRange{pg::kUnbounded, pg::kUnbounded}),
            (pg::IntegerRange{pg::kUnbounded, pg::kUnbounded}));
  EXPECT_NE(pg::IntegerRange{},
            (pg::IntegerRange{pg::kUnbounded, pg::kUnbounded}));

  EXPECT_EQ(pg::MakeRange(1, 2), pg::MakeRange(1, 2));
  EXPECT_NE(pg::MakeRange(1, 2), pg::MakeRange(1, 3));
  EXPECT_NE(pg::MakeRange(1, 2), pg::MakeRange(1, 2, pg::RangeBound::kNone));
  EXPECT_NE(pg::MakeRange(1, 2), pg::MakeRange(1, 2, pg::RangeBound::kBoth));
  EXPECT_NE(pg::MakeRange(1, 2), pg::MakeRange(1, 2, pg::RangeBound::kUpper));

  EXPECT_EQ(pg::MakeRange(1, 2), pg::MakeRange(0, 2, pg::RangeBound::kNone));
  EXPECT_EQ(pg::MakeRange(1, 4), pg::MakeRange(1, 3, pg::RangeBound::kBoth));

  EXPECT_EQ(pg::MakeRange(0, pg::kUnbounded), pg::MakeRange(0, pg::kUnbounded));
  EXPECT_EQ(pg::MakeRange(1, pg::kUnbounded),
            pg::MakeRange(0, pg::kUnbounded, pg::RangeBound::kNone));

  EXPECT_EQ(pg::MakeRange(pg::kUnbounded, 0), pg::MakeRange(pg::kUnbounded, 0));
  EXPECT_EQ(pg::MakeRange(pg::kUnbounded, 1),
            pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kUpper));
}

template <typename RangeType>
struct TestData {
  std::string select_expression;
  std::string description;
  RangeType expected;
};

UTEST_P(PostgreConnection, Int4RangeRoundtripTest) {
  CheckConnection(GetConn());
  ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  pg::IntegerRange r;

  TestData<pg::IntegerRange> test_data[]{
      {"empty", "empty", pg::IntegerRange{}},
      {"[0,2)", "lower-inclusive", pg::MakeRange(0, 2)},
      {"(0, 2)", "non-inclusive", pg::MakeRange(0, 2, pg::RangeBound::kNone)},
      {"(0, 2]", "upper-inclusive",
       pg::MakeRange(0, 2, pg::RangeBound::kUpper)},
      {"[0, 2]", "both-inclusive", pg::MakeRange(0, 2, pg::RangeBound::kBoth)},
      {"(,)", "unbounded", pg::IntegerRange{pg::kUnbounded, pg::kUnbounded}},
      {"[0,)", "upper-unbounded-inclusive", pg::MakeRange(0, pg::kUnbounded)},
      {"(,0]", "lower-unbounded-inclusive",
       pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kUpper)},
      {"(0,)", "upper-unbounded-exclusive",
       pg::MakeRange(0, pg::kUnbounded, pg::RangeBound::kNone)},
      {"(,0)", "lower-unbounded-exclusive",
       pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kNone)}};

  for (const auto& test : test_data) {
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select '" + test.select_expression +
                                 "'::int4range, '" + test.description + "'"))
        << "Select " << test.description << " range from database";
    UEXPECT_NO_THROW(res.Front()[0].To(r))
        << "Parse " << test.select_expression << " value";
    EXPECT_EQ(test.expected, r)
        << "Expect equality for " << test.description << " range";

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", test.expected))
        << "Roundtrip select " << test.description << " range";
    UEXPECT_NO_THROW(res.Front()[0].To(r));
    EXPECT_EQ(test.expected, r)
        << "Expect equality for " << test.description << " range";
  }
}

UTEST_P(PostgreConnection, Int8RangeRoundtripTest) {
  CheckConnection(GetConn());
  ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

  pg::ResultSet res{nullptr};
  pg::BigintRange r;

  TestData<pg::BigintRange> test_data[]{
      {"empty", "empty", pg::BigintRange{}},
      {"[0,2)", "lower-inclusive", pg::MakeRange(0, 2)},
      {"(0, 2)", "non-inclusive", pg::MakeRange(0, 2, pg::RangeBound::kNone)},
      {"(0, 2]", "upper-inclusive",
       pg::MakeRange(0, 2, pg::RangeBound::kUpper)},
      {"[0, 2]", "both-inclusive", pg::MakeRange(0, 2, pg::RangeBound::kBoth)},
      {"(,)", "unbounded", pg::BigintRange{pg::kUnbounded, pg::kUnbounded}},
      {"[0,)", "upper-unbounded-inclusive", pg::MakeRange(0, pg::kUnbounded)},
      {"(,0]", "lower-unbounded-inclusive",
       pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kUpper)},
      {"(0,)", "upper-unbounded-exclusive",
       pg::MakeRange(0, pg::kUnbounded, pg::RangeBound::kNone)},
      {"(,0)", "lower-unbounded-exclusive",
       pg::MakeRange(pg::kUnbounded, 0, pg::RangeBound::kNone)}};

  for (const auto& test : test_data) {
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select '" + test.select_expression +
                                 "'::int8range, '" + test.description + "'"))
        << "Select " << test.description << " range from database";
    UEXPECT_NO_THROW(res.Front()[0].To(r))
        << "Parse " << test.select_expression << " value";
    EXPECT_EQ(test.expected, r)
        << "Expect equality for " << test.description << " range";

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", test.expected))
        << "Roundtrip select " << test.description << " range";
    UEXPECT_NO_THROW(res.Front()[0].To(r));
    EXPECT_EQ(test.expected, r)
        << "Expect equality for " << test.description << " range";
  }
}

UTEST_P(PostgreConnection, BoundedInt8RangeRoundtripTest) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  std::string invalid_ranges[]{"empty", "(,)", "[0,)", "(,0]", "(0,)", "(,0)"};
  for (const auto& test : invalid_ranges) {
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select '" + test + "'::int8range"));
    UEXPECT_THROW(res.AsSingleRow<pg::BoundedBigintRange>(),
                  pg::BoundedRangeError);
  }
  TestData<pg::BoundedBigintRange> test_data[]{
      {"[0,2)", "lower-inclusive", pg::BoundedBigintRange(0L, 2L)},
      {"(0, 2)", "non-inclusive",
       pg::BoundedBigintRange(0L, 2L, pg::RangeBound::kNone)},
      {"(0, 2]", "upper-inclusive",
       pg::BoundedBigintRange(0L, 2L, pg::RangeBound::kUpper)},
      {"[0, 2]", "both-inclusive",
       pg::BoundedBigintRange(0L, 2L, pg::RangeBound::kBoth)},
  };
  for (const auto& test : test_data) {
    pg::BoundedBigintRange r;
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select '" + test.select_expression +
                                 "'::int8range, '" + test.description + "'"))
        << "Select " << test.description << " range from database";
    UEXPECT_NO_THROW(res.Front()[0].To(r))
        << "Parse " << test.select_expression << " value";
    EXPECT_EQ(test.expected, r)
        << "Expect equality for " << test.description << " range";

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", test.expected))
        << "Roundtrip select " << test.description << " range";
    UEXPECT_NO_THROW(res.Front()[0].To(r));
    EXPECT_EQ(test.expected, r)
        << "Expect equality for " << test.description << " range";
  }
}

UTEST_P(PostgreConnection, RangeStored) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  auto exp1 = pg::MakeRange(-1, 1, pg::RangeBound::kLower);
  auto exp2 = pg::MakeRange(int64_t{13}, int64_t{42});
  UEXPECT_NO_THROW(
      res = GetConn()->Execute(
          "select $1, $2", pg::ParameterStore{}.PushBack(exp1).PushBack(exp2)));

  EXPECT_EQ(exp1, res[0][0].As<pg::IntegerRange>());
  EXPECT_EQ(exp2, res[0][1].As<pg::BigintRange>());
}

}  // namespace

USERVER_NAMESPACE_END
