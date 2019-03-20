#include <gtest/gtest.h>

#include <boost/optional.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include <formats/parse/to.hpp>

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
  auto value = this->FromString("null");

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
  auto value = this->FromString("null");
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
  auto value = this->FromString("null");
  std::vector<std::vector<int>> v =
      value.template As<std::vector<std::vector<int>>>();
  EXPECT_EQ(std::vector<std::vector<int>>{}, v);
}

TYPED_TEST_P(Parsing, OptionalIntNone) {
  auto value = this->FromString("{}")["nonexisting"];
  boost::optional<int> v = value.template As<boost::optional<int>>();
  EXPECT_EQ(boost::none, v);
}

TYPED_TEST_P(Parsing, OptionalInt) {
  auto value = this->FromString("[1]")[0];
  boost::optional<int> v = value.template As<boost::optional<int>>();
  EXPECT_EQ(1, v);
}

TYPED_TEST_P(Parsing, OptionalVectorInt) {
  auto value = this->FromString("{}")["nonexisting"];
  boost::optional<std::vector<int>> v =
      value.template As<boost::optional<std::vector<int>>>();
  EXPECT_EQ(boost::none, v);
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

REGISTER_TYPED_TEST_CASE_P(Parsing,

                           ContainersCtr, VectorInt, VectorIntNull,
                           VectorIntErrorObj, VectorVectorInt,
                           VectorVectorIntNull, OptionalIntNone,

                           OptionalInt, OptionalVectorInt, Int, UInt,
                           IntOverflow, UserProvidedCommonParser,

                           ChronoSeconds);
