#include <userver/formats/parse/boost_flat_containers.hpp>
#include <userver/formats/parse/boost_optional.hpp>
#include <userver/formats/parse/time_of_day.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/parse/variant.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

namespace boost {
template <class T>
void PrintTo(const optional<T>& from, std::ostream* os) {
  if (from) {
    *os << testing::PrintToString(*from);
  } else {
    *os << "nullopt";
  }
}
}  // namespace boost

USERVER_NAMESPACE_BEGIN

namespace testing_namespace {
struct TestType {
  int i;
};

template <class Value>
inline TestType Parse(const Value& val, formats::parse::To<TestType>) {
  return TestType{val.template As<int>()};
}
}  // namespace testing_namespace

namespace testing_namespace2 {
struct TestType {
  int i;
};
}  // namespace testing_namespace2

namespace formats::parse {
template <class Value>
testing_namespace2::TestType Parse(const Value& val,
                                   To<testing_namespace2::TestType>) {
  return testing_namespace2::TestType{val.template As<int>()};
}
}  // namespace formats::parse

template <class T>
struct Parsing : public ::testing::Test {};

TYPED_TEST_SUITE_P(Parsing);

TYPED_TEST_P(Parsing, ContainersCtr) {
  auto value = this->FromString(R"({"null": null})")["null"];

  EXPECT_TRUE(value.template As<std::unordered_set<int>>().empty());
  EXPECT_TRUE(value.template As<std::vector<int>>().empty());
  EXPECT_TRUE(value.template As<std::set<int>>().empty());
  EXPECT_TRUE((value.template As<std::map<std::string, int>>().empty()));
  EXPECT_TRUE(
      (value.template As<std::unordered_map<std::string, int>>().empty()));
}

TYPED_TEST_P(Parsing, VectorInt) {
  auto value = this->FromString("[1,2,3]");
  auto v = value.template As<std::vector<int>>();
  EXPECT_EQ((std::vector<int>{1, 2, 3}), v);
}

TYPED_TEST_P(Parsing, VectorIntNull) {
  auto value = this->FromString(R"({"null": null})")["null"];
  auto v = value.template As<std::vector<int>>();
  EXPECT_EQ(std::vector<int>{}, v);
}

TYPED_TEST_P(Parsing, VectorIntErrorObj) {
  auto value = this->FromString("{\"1\": 1}");
  EXPECT_THROW(value.template As<std::vector<int>>(), std::exception);
}

TYPED_TEST_P(Parsing, VectorVectorInt) {
  auto value = this->FromString("[[1,2,3],[4,5],[6],[]]");
  auto v = value.template As<std::vector<std::vector<int>>>();
  EXPECT_EQ((std::vector<std::vector<int>>{{1, 2, 3}, {4, 5}, {6}, {}}), v);
}

TYPED_TEST_P(Parsing, VectorVectorIntNull) {
  auto value = this->FromString(R"({"null": null})")["null"];
  auto v = value.template As<std::vector<std::vector<int>>>();
  EXPECT_EQ(std::vector<std::vector<int>>{}, v);
}

TYPED_TEST_P(Parsing, BoostContainerFlatSet) {
  auto value = this->FromString("[3,3,5,2,1,5,6,4]");
  auto v = value.template As<boost::container::flat_set<int>>();
  EXPECT_EQ(6, v.size());
  for (std::size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(*v.nth(i), i + 1) << i;
  }
}

TYPED_TEST_P(Parsing, BoostContainerFlatMap) {
  auto value = this->FromString(R"({"b":1, "a":0})");
  auto v = value.template As<boost::container::flat_map<std::string, int>>();
  EXPECT_EQ(2, v.size());
  for (std::size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(v.nth(i)->second, i) << i;
  }
}

TYPED_TEST_P(Parsing, BoostOptionalIntNone) {
  auto value = this->FromString("{}")["nonexisting"];
  auto v = value.template As<boost::optional<int>>();
  EXPECT_EQ(boost::none, v);
  EXPECT_FALSE(value.template As<std::optional<int>>());
}

TYPED_TEST_P(Parsing, BoostOptionalInt) {
  auto value = this->FromString("[1]")[0];
  auto v = value.template As<boost::optional<int>>();
  EXPECT_EQ(1, v);
  EXPECT_EQ(1, value.template As<std::optional<int>>());
}

TYPED_TEST_P(Parsing, BoostOptionalVectorInt) {
  auto value = this->FromString("{}")["nonexisting"];
  auto v = value.template As<boost::optional<std::vector<int>>>();
  EXPECT_EQ(boost::none, v);
  EXPECT_FALSE(value.template As<std::optional<std::vector<int>>>());
}

TYPED_TEST_P(Parsing, OptionalIntNone) {
  auto value = this->FromString("{}")["nonexisting"];
  auto v = value.template As<std::optional<int>>();
  EXPECT_EQ(std::nullopt, v);
}

TYPED_TEST_P(Parsing, OptionalInt) {
  auto value = this->FromString("[1]")[0];
  auto v = value.template As<std::optional<int>>();
  EXPECT_EQ(1, v);
}

TYPED_TEST_P(Parsing, OptionalVectorInt) {
  auto value = this->FromString("{}")["nonexisting"];
  auto v = value.template As<std::optional<std::vector<int>>>();
  EXPECT_EQ(std::nullopt, v);
}

TYPED_TEST_P(Parsing, Int) {
  auto value = this->FromString("[32768]")[0];
  EXPECT_THROW(value.template As<int16_t>(), std::exception);

  value = this->FromString("[32767]")[0];
  EXPECT_EQ(32767, value.template As<int16_t>());
}

TYPED_TEST_P(Parsing, UInt) {
  auto value = this->FromString("[-1]")[0];
  EXPECT_THROW(value.template As<unsigned int>(), std::exception);

  value = this->FromString("[0]")[0];
  EXPECT_EQ(0, value.template As<unsigned int>());
}

TYPED_TEST_P(Parsing, IntOverflow) {
  auto value = this->FromString("[65536]")[0];
  EXPECT_THROW(value.template As<uint16_t>(), std::exception);

  value = this->FromString("[65535]")[0];
  EXPECT_EQ(65535, value.template As<uint16_t>());
}

TYPED_TEST_P(Parsing, UserProvidedCommonParser) {
  auto value = this->FromString("[42]")[0];

  const auto converted = value.template As<testing_namespace::TestType>();
  EXPECT_EQ(converted.i, 42);

  const auto converted2 = value.template As<testing_namespace2::TestType>();
  EXPECT_EQ(converted2.i, 42);
}

TYPED_TEST_P(Parsing, ChronoDoubleSeconds) {
  auto value = this->FromString("[10.1]")[0];

  auto converted = value.template As<std::chrono::duration<double>>();
  EXPECT_DOUBLE_EQ(converted.count(),
                   std::chrono::duration<double>{10.1}.count());
}

TYPED_TEST_P(Parsing, ChronoSeconds) {
  auto value = this->FromString("[\"10h\"]")[0];

  {
    const auto converted = value.template As<std::chrono::seconds>();
    EXPECT_EQ(converted, std::chrono::hours(10));
  }

  {
    value = this->FromString("[10]")[0];

    const auto converted = value.template As<std::chrono::seconds>();
    EXPECT_EQ(converted, std::chrono::seconds{10});
  }
}

TYPED_TEST_P(Parsing, VariantOk1) {
  using Variant = std::variant<std::string, int>;

  auto value = this->FromString("[\"string\"]")[0];
  const auto converted = value.template As<Variant>();
  EXPECT_EQ(Variant("string"), converted);
}

TYPED_TEST_P(Parsing, VariantOk2) {
  using Variant = std::variant<std::vector<int>, int>;

  auto value = this->FromString("[10]")[0];
  const auto converted = value.template As<Variant>();
  // Don't use EXPECT_EQ() below as there is no operator<<(ostream&, const
  // std::vector<T>&)
  EXPECT_TRUE(Variant(10) == converted);
}

TYPED_TEST_P(Parsing, VariantAmbiguous) {
  using Variant = std::variant<long, int>;
  using ParseException = typename TestFixture::ParseException;

  auto value = this->FromString("[10]")[0];
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(value.template As<Variant>(), ParseException);
}

TYPED_TEST_P(Parsing, VariantUnambiguous) {
  using Variant = std::variant<unsigned, signed>;

  auto value = this->FromString("[-10]")[0];
  const auto converted = value.template As<Variant>();
  EXPECT_EQ(Variant(-10), converted);
  EXPECT_EQ(1, converted.index());
}

TYPED_TEST_P(Parsing, TimeOfDayCorrect) {
  using Minutes = utils::datetime::TimeOfDay<std::chrono::minutes>;

  auto json = this->FromString(R"~(["6:30"])~")[0];
  const auto value = json.template As<Minutes>();
  EXPECT_EQ(Minutes{"06:30"}, value);
}

TYPED_TEST_P(Parsing, TimeOfDayIncorrect) {
  using Minutes = utils::datetime::TimeOfDay<std::chrono::minutes>;
  using ParseException = typename TestFixture::ParseException;

  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromString(R"~(["6"])~")[0].template As<Minutes>(),
               ParseException);
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromString(R"~(["6:"])~")[0].template As<Minutes>(),
               ParseException);
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromString(R"~(["6:60"])~")[0].template As<Minutes>(),
               ParseException);
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromString(R"~(["24:01"])~")[0].template As<Minutes>(),
               ParseException);
}

TYPED_TEST_P(Parsing, TimeOfDayNormalized) {
  using Minutes = utils::datetime::TimeOfDay<std::chrono::minutes>;

  auto json = this->FromString(R"~(["24:00"])~")[0];
  const auto value = json.template As<Minutes>();
  EXPECT_EQ(Minutes{"00:00"}, value);
}

struct DontDefaultMe {
  DontDefaultMe() { Fail(); }
  explicit DontDefaultMe(int value) : value(value) {}
  int value = 0;

  static void Fail() { FAIL() << "Extra default constructor invoked by As"; }
};

template <typename Value>
DontDefaultMe Parse(const Value& value, formats::parse::To<DontDefaultMe>) {
  return DontDefaultMe{value.template As<int>()};
}

TYPED_TEST_P(Parsing, AsDefaulted) {
  auto json = this->FromString(R"~({"foo": 42, "bar": [9000]})~");

  EXPECT_EQ(json["foo"].template As<int>({}), 42);
  EXPECT_EQ(json["bar"].template As<std::vector<int>>({}),
            std::vector<int>{9000});
  EXPECT_EQ(json["foo"].template As<DontDefaultMe>({}).value, 42);

  EXPECT_EQ(json["missing"].template As<int>({}), 0);
  EXPECT_EQ(json["missing"].template As<std::vector<int>>({}),
            std::vector<int>{});
}

REGISTER_TYPED_TEST_SUITE_P(
    Parsing,

    ContainersCtr, VectorInt, VectorIntNull, VectorIntErrorObj, VectorVectorInt,
    VectorVectorIntNull,

    BoostContainerFlatSet, BoostContainerFlatMap,

    BoostOptionalIntNone, BoostOptionalInt, BoostOptionalVectorInt,
    OptionalIntNone, OptionalInt, OptionalVectorInt,

    Int, UInt, IntOverflow, UserProvidedCommonParser,

    ChronoDoubleSeconds, ChronoSeconds,

    VariantOk1, VariantOk2, VariantAmbiguous, VariantUnambiguous,

    TimeOfDayCorrect, TimeOfDayIncorrect, TimeOfDayNormalized,

    AsDefaulted);

USERVER_NAMESPACE_END
