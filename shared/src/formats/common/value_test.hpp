#include <gtest/gtest.h>

#include <boost/optional.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <formats/parse/to.hpp>
#include <formats/parse/variant.hpp>

namespace {
namespace testing_namespace {
struct TestType {
  int i;
};

template <class Value>
TestType Parse(const Value& val, formats::parse::To<TestType>) {
  return TestType{val.template As<int>()};
}
}  // namespace testing_namespace

namespace testing_namespace2 {
struct TestType {
  int i;
};
}  // namespace testing_namespace2
}  // namespace

namespace formats::parse {
template <class Value>
testing_namespace2::TestType Parse(const Value& val,
                                   To<testing_namespace2::TestType>) {
  return testing_namespace2::TestType{val.template As<int>()};
}
}  // namespace formats::parse

template <class T>
struct Parsing : public ::testing::Test {};

TYPED_TEST_CASE_P(Parsing);

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
  std::vector<int> v = value.template As<std::vector<int>>();
  EXPECT_EQ((std::vector<int>{1, 2, 3}), v);
}

TYPED_TEST_P(Parsing, VectorIntNull) {
  auto value = this->FromString(R"({"null": null})")["null"];
  std::vector<int> v = value.template As<std::vector<int>>();
  EXPECT_EQ(std::vector<int>{}, v);
}

TYPED_TEST_P(Parsing, VectorIntErrorObj) {
  auto value = this->FromString("{\"1\": 1}");
  EXPECT_THROW(value.template As<std::vector<int>>(), std::exception);
}

TYPED_TEST_P(Parsing, VectorVectorInt) {
  auto value = this->FromString("[[1,2,3],[4,5],[6],[]]");
  std::vector<std::vector<int>> v =
      value.template As<std::vector<std::vector<int>>>();
  EXPECT_EQ((std::vector<std::vector<int>>{{1, 2, 3}, {4, 5}, {6}, {}}), v);
}

TYPED_TEST_P(Parsing, VectorVectorIntNull) {
  auto value = this->FromString(R"({"null": null})")["null"];
  std::vector<std::vector<int>> v =
      value.template As<std::vector<std::vector<int>>>();
  EXPECT_EQ(std::vector<std::vector<int>>{}, v);
}

TYPED_TEST_P(Parsing, BoostOptionalIntNone) {
  auto value = this->FromString("{}")["nonexisting"];
  boost::optional<int> v = value.template As<boost::optional<int>>();
  EXPECT_EQ(boost::none, v);
  EXPECT_FALSE(value.template As<std::optional<int>>());
}

TYPED_TEST_P(Parsing, BoostOptionalInt) {
  auto value = this->FromString("[1]")[0];
  boost::optional<int> v = value.template As<boost::optional<int>>();
  EXPECT_EQ(1, v);
  EXPECT_EQ(1, value.template As<std::optional<int>>());
}

TYPED_TEST_P(Parsing, BoostOptionalVectorInt) {
  auto value = this->FromString("{}")["nonexisting"];
  boost::optional<std::vector<int>> v =
      value.template As<boost::optional<std::vector<int>>>();
  EXPECT_EQ(boost::none, v);
  EXPECT_FALSE(value.template As<std::optional<std::vector<int>>>());
}

TYPED_TEST_P(Parsing, OptionalIntNone) {
  auto value = this->FromString("{}")["nonexisting"];
  std::optional<int> v = value.template As<std::optional<int>>();
  EXPECT_EQ(std::nullopt, v);
}

TYPED_TEST_P(Parsing, OptionalInt) {
  auto value = this->FromString("[1]")[0];
  std::optional<int> v = value.template As<std::optional<int>>();
  EXPECT_EQ(1, v);
}

TYPED_TEST_P(Parsing, OptionalVectorInt) {
  auto value = this->FromString("{}")["nonexisting"];
  std::optional<std::vector<int>> v =
      value.template As<std::optional<std::vector<int>>>();
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
  EXPECT_EQ(65535u, value.template As<uint16_t>());
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
  EXPECT_THROW(value.template As<Variant>(), ParseException);
}

TYPED_TEST_P(Parsing, VariantUnambiguous) {
  using Variant = std::variant<unsigned, signed>;

  auto value = this->FromString("[-10]")[0];
  const auto converted = value.template As<Variant>();
  EXPECT_EQ(Variant(-10), converted);
  EXPECT_EQ(1, converted.index());
}

REGISTER_TYPED_TEST_CASE_P(Parsing,

                           ContainersCtr, VectorInt, VectorIntNull,
                           VectorIntErrorObj, VectorVectorInt,
                           VectorVectorIntNull,

                           BoostOptionalIntNone, BoostOptionalInt,
                           BoostOptionalVectorInt, OptionalIntNone, OptionalInt,
                           OptionalVectorInt,

                           Int, UInt, IntOverflow, UserProvidedCommonParser,

                           ChronoDoubleSeconds, ChronoSeconds,

                           VariantOk1, VariantOk2, VariantAmbiguous,
                           VariantUnambiguous

);
