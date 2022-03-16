#include <userver/formats/common/utils.hpp>

#include <utility>

#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ValueTypes = ::testing::Types<formats::json::Value, formats::yaml::Value>;

template <typename T>
class FormatsFixture;

template <>
class FormatsFixture<formats::yaml::Value> : public ::testing::Test {
 public:
  using Exception = formats::yaml::TypeMismatchException;
};

template <>
class FormatsFixture<formats::json::Value> : public ::testing::Test {
 public:
  using Exception = formats::json::TypeMismatchException;
};

}  // namespace

template <typename T>
using FormatsGetAtPathValueBuilder = FormatsFixture<T>;

TYPED_TEST_SUITE(FormatsGetAtPathValueBuilder, ValueTypes);

TYPED_TEST(FormatsGetAtPathValueBuilder, One) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  std::vector<std::string> path = {"key1"};
  ValueBuilder result_builder;
  result_builder = formats::common::GetAtPath(origin, std::move(path));
  auto result = result_builder.ExtractValue();
  ASSERT_EQ(result, ValueBuilder(1).ExtractValue());
}

TYPED_TEST(FormatsGetAtPathValueBuilder, NonObjectElemOnPath) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  std::vector<std::string> path = {"key1", "key2"};
  ASSERT_THROW(formats::common::GetAtPath(origin, std::move(path)),
               typename TestFixture::Exception);
}

TYPED_TEST(FormatsGetAtPathValueBuilder, Odd) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  std::vector<std::string> path = {"key1", "key2", "key3"};
  ValueBuilder result_builder;
  result_builder = formats::common::GetAtPath(origin, std::move(path));
  auto result = result_builder.ExtractValue();
  ASSERT_EQ(result, ValueBuilder(1).ExtractValue());
}

TYPED_TEST(FormatsGetAtPathValueBuilder, Even) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"] = 1;
  std::vector<std::string> path = {"key1", "key2"};

  ValueBuilder result_builder;
  result_builder = formats::common::GetAtPath(origin, std::move(path));
  auto result = result_builder.ExtractValue();
  ASSERT_EQ(result, ValueBuilder(1).ExtractValue());
}

template <typename T>
using FormatsGetAtPathValue = FormatsFixture<T>;

TYPED_TEST_SUITE(FormatsGetAtPathValue, ValueTypes);

TYPED_TEST(FormatsGetAtPathValue, Empty) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  Value result = origin.ExtractValue();
  const std::vector<std::string> path{};
  ASSERT_EQ(formats::common::GetAtPath(result, path), result);
}

TYPED_TEST(FormatsGetAtPathValue, NonObjectElemOnPath) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  Value origin_value = origin.ExtractValue();
  std::vector<std::string> path = {"key1", "key2"};
  ASSERT_THROW(formats::common::GetAtPath(origin_value, std::move(path)),
               typename TestFixture::Exception);
}

TYPED_TEST(FormatsGetAtPathValue, One) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  Value result = origin.ExtractValue();
  const std::vector<std::string> path{"key1"};
  ASSERT_EQ(formats::common::GetAtPath(result, path), result["key1"]);
}

TYPED_TEST(FormatsGetAtPathValue, Odd) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder builder;
  ValueBuilder new_elem = 2;
  builder["key1"]["key2"]["key3"] = new_elem;
  Value result = builder.ExtractValue();
  const std::vector<std::string> path{"key1", "key2", "key3"};
  ASSERT_EQ(formats::common::GetAtPath(result, path), new_elem.ExtractValue());
}

TYPED_TEST(FormatsGetAtPathValue, Even) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder builder;
  builder["key1"]["key2"]["key3"] = 1;
  Value result = builder.ExtractValue();
  const std::vector<std::string> path{"key1", "key2"};
  ASSERT_EQ(formats::common::GetAtPath(result, path), result["key1"]["key2"]);
}

template <typename T>
using FormatsSetAtPath = FormatsFixture<T>;

TYPED_TEST_SUITE(FormatsSetAtPath, ValueTypes);

TYPED_TEST(FormatsSetAtPath, Empty) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path;
  ValueBuilder origin_test_empty;
  formats::common::SetAtPath(origin_test_empty, std::move(path), new_elem);
  ASSERT_EQ(origin_test_empty.ExtractValue(), new_elem);
}

TYPED_TEST(FormatsSetAtPath, NonObjectElemOnPath) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"] = "2";
  ASSERT_THROW(formats::common::SetAtPath(origin, std::move(path), new_elem),
               typename TestFixture::Exception);
}

TYPED_TEST(FormatsSetAtPath, One) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"], ValueBuilder(2).ExtractValue());
}

TYPED_TEST(FormatsSetAtPath, Odd) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1", "key2", "key3"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"]["key2"]["key3"], ValueBuilder(2).ExtractValue());
}

TYPED_TEST(FormatsSetAtPath, Even) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1", "key2"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"]["key2"], ValueBuilder(2).ExtractValue());
}

template <typename T>
using FormatsRemoveAtPath = FormatsFixture<T>;

TYPED_TEST_SUITE(FormatsRemoveAtPath, ValueTypes);

TYPED_TEST(FormatsRemoveAtPath, Empty) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path;
  origin["key1"] = 1;
  formats::common::RemoveAtPath(origin, std::move(path));
  ASSERT_TRUE(origin.IsEmpty());
}

TYPED_TEST(FormatsRemoveAtPath, One) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1"};
  origin["key1"] = 1;
  origin["key2"] = 2;
  formats::common::RemoveAtPath(origin, std::move(path));
  const auto result = origin.ExtractValue();
  ASSERT_FALSE(result.HasMember("key1"));
  ASSERT_TRUE(result.HasMember("key2"));
}

TYPED_TEST(FormatsRemoveAtPath, Even) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2"};
  origin["key1"]["key2"] = 1;
  formats::common::RemoveAtPath(origin, std::move(path));
  const auto result = origin.ExtractValue();
  ASSERT_TRUE(result.HasMember("key1"));
  ASSERT_FALSE(result["key1"].HasMember("key2"));
}

TYPED_TEST(FormatsRemoveAtPath, Odd) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"]["key3"] = "2";
  formats::common::RemoveAtPath(origin, std::move(path));
  const auto result = origin.ExtractValue();
  ASSERT_TRUE(result["key1"].HasMember("key2"));
  ASSERT_FALSE(result["key1"]["key2"].HasMember("key3"));
}

TYPED_TEST(FormatsRemoveAtPath, NonObjectElemOnPath) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"] = "2";
  ASSERT_THROW(formats::common::RemoveAtPath(origin, std::move(path)),
               typename TestFixture::Exception);
}

TEST(FormatsSplitPath, Empty) {
  ASSERT_TRUE(formats::common::SplitPathString("").empty());
}

TEST(FormatsSplitPath, Many) {
  std::vector<std::string> result =
      formats::common::SplitPathString("key1.key2.sample");
  ASSERT_EQ((int)(result.size()), 3);
  ASSERT_EQ(result[0], "key1");
  ASSERT_EQ(result[1], "key2");
  ASSERT_EQ(result[2], "sample");
}

USERVER_NAMESPACE_END
