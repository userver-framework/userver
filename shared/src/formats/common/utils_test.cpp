#include <userver/formats/common/utils.hpp>

#include <utility>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

using formats::json::Value;
using formats::json::ValueBuilder;

TEST(FormatsGetAtPathValueBuilder, One) {
  ValueBuilder origin;
  origin["key1"] = 1;
  std::vector<std::string> path = {"key1"};
  ValueBuilder result;
  result = formats::common::GetAtPath(origin, std::move(path));
  ASSERT_TRUE(result.ExtractValue().As<int>() == 1);
}

TEST(FormatsGetAtPathValueBuilder, NonObjectElemOnPath) {
  ValueBuilder origin;
  origin["key1"] = 1;
  std::vector<std::string> path = {"key1", "key2"};
  ValueBuilder result;
  ASSERT_THROW(formats::common::GetAtPath(origin, std::move(path)),
               formats::json::TypeMismatchException);
}

TEST(FormatsGetAtPathValueBuilder, Odd) {
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  std::vector<std::string> path = {"key1", "key2", "key3"};
  ValueBuilder result;
  result = formats::common::GetAtPath(origin, std::move(path));
  ASSERT_TRUE(result.ExtractValue().As<int>() == 1);
}

TEST(FormatsGetAtPathValueBuilder, Even) {
  ValueBuilder origin;
  origin["key1"]["key2"] = 1;
  std::vector<std::string> path = {"key1", "key2"};
  ValueBuilder result;
  result = formats::common::GetAtPath(origin, std::move(path));
  ASSERT_TRUE(result.ExtractValue().As<int>() == 1);
}

TEST(FormatsGetAtPathValue, Empty) {
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  Value result = origin.ExtractValue();
  const std::vector<std::string> path{};
  ASSERT_TRUE(formats::common::GetAtPath(result, path) == result);
}

TEST(FormatsGetAtPathValue, NonObjectElemOnPath) {
  ValueBuilder origin;
  origin["key1"] = 1;
  Value origin_value = origin.ExtractValue();
  std::vector<std::string> path = {"key1", "key2"};
  ValueBuilder result;
  ASSERT_THROW(formats::common::GetAtPath(origin_value, std::move(path)),
               formats::json::TypeMismatchException);
}

TEST(FormatsGetAtPathValue, One) {
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  Value result = origin.ExtractValue();
  const std::vector<std::string> path{"key1"};
  ASSERT_TRUE(formats::common::GetAtPath(result, path) == result["key1"]);
}

TEST(FormatsGetAtPathValue, Odd) {
  ValueBuilder builder;
  builder["key1"]["key2"]["key3"] = 1;
  Value result = builder.ExtractValue();
  const std::vector<std::string> path{"key1", "key2", "key3"};
  ASSERT_EQ(formats::common::GetAtPath(result, path).As<int>(), 1);
}

TEST(FormatsGetAtPathValue, Even) {
  ValueBuilder builder;
  builder["key1"]["key2"]["key3"] = 1;
  Value result = builder.ExtractValue();
  const std::vector<std::string> path{"key1", "key2"};
  ASSERT_TRUE(formats::common::GetAtPath(result, path) ==
              result["key1"]["key2"]);
}

TEST(FormatsSetAtPath, Empty) {
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path;
  ValueBuilder origin_test_empty;
  formats::common::SetAtPath(origin_test_empty, std::move(path), new_elem);
  ASSERT_TRUE(origin_test_empty.ExtractValue() == new_elem);
}

TEST(FormatsSetAtPath, NonObjectElemOnPath) {
  const Value new_elem = ValueBuilder(2).ExtractValue();
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"] = "2";
  ASSERT_THROW(formats::common::SetAtPath(origin, std::move(path), new_elem),
               formats::json::TypeMismatchException);
}

TEST(FormatsSetAtPath, One) {
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"].As<int>(), 2);
}

TEST(FormatsSetAtPath, Odd) {
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1", "key2", "key3"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"]["key2"]["key3"].As<int>(), 2);
}

TEST(FormatsSetAtPath, Even) {
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1", "key2"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"]["key2"].As<int>(), 2);
}

TEST(FormatsRemoveAtPath, Empty) {
  ValueBuilder origin;
  std::vector<std::string> path;
  origin["key1"] = 1;
  formats::common::RemoveAtPath(origin, std::move(path));
  const Value result_test_empty = origin.ExtractValue();
  ASSERT_TRUE(result_test_empty == Value());
}

TEST(FormatsRemoveAtPath, One) {
  ValueBuilder origin;
  std::vector<std::string> path{"key1"};
  origin["key1"] = 1;
  origin["key2"] = 2;
  formats::common::RemoveAtPath(origin, std::move(path));
  const Value result = origin.ExtractValue();
  ASSERT_FALSE(result.HasMember("key1"));
  ASSERT_TRUE(result.HasMember("key2"));
}

TEST(FormatsRemoveAtPath, Even) {
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2"};
  origin["key1"]["key2"] = 1;
  formats::common::RemoveAtPath(origin, std::move(path));
  const Value result = origin.ExtractValue();
  ASSERT_TRUE(result.HasMember("key1"));
  ASSERT_FALSE(result["key1"].HasMember("key2"));
}

TEST(FormatsRemoveAtPath, Odd) {
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"]["key3"] = "2";
  formats::common::RemoveAtPath(origin, std::move(path));
  const Value result = origin.ExtractValue();
  ASSERT_TRUE(result["key1"].HasMember("key2"));
  ASSERT_FALSE(result["key1"]["key2"].HasMember("key3"));
}

TEST(FormatsRemoveAtPath, NonObjectElemOnPath) {
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"] = "2";
  ASSERT_THROW(formats::common::RemoveAtPath(origin, std::move(path)),
               formats::json::TypeMismatchException);
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
