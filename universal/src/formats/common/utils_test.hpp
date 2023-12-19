#include <userver/formats/common/utils.hpp>

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

template <typename T>
class FormatsUtils;

template <typename T>
class FormatsGetAtPathValueBuilder : public FormatsUtils<T> {};

TYPED_TEST_SUITE_P(FormatsGetAtPathValueBuilder);

TYPED_TEST_P(FormatsGetAtPathValueBuilder, One) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  std::vector<std::string> path = {"key1"};
  ValueBuilder result_builder;
  result_builder = formats::common::GetAtPath(origin, std::move(path));
  auto result = result_builder.ExtractValue();
  ASSERT_EQ(result, ValueBuilder(1).ExtractValue());
}

TYPED_TEST_P(FormatsGetAtPathValueBuilder, NonObjectElemOnPath) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  std::vector<std::string> path = {"key1", "key2"};
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  ASSERT_THROW(formats::common::GetAtPath(origin, std::move(path)),
               typename TestFixture::Exception);
}

TYPED_TEST_P(FormatsGetAtPathValueBuilder, Odd) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  std::vector<std::string> path = {"key1", "key2", "key3"};
  ValueBuilder result_builder;
  result_builder = formats::common::GetAtPath(origin, std::move(path));
  auto result = result_builder.ExtractValue();
  ASSERT_EQ(result, ValueBuilder(1).ExtractValue());
}

TYPED_TEST_P(FormatsGetAtPathValueBuilder, Even) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"] = 1;
  std::vector<std::string> path = {"key1", "key2"};

  ValueBuilder result_builder;
  result_builder = formats::common::GetAtPath(origin, std::move(path));
  auto result = result_builder.ExtractValue();
  ASSERT_EQ(result, ValueBuilder(1).ExtractValue());
}

REGISTER_TYPED_TEST_SUITE_P(FormatsGetAtPathValueBuilder, One,
                            NonObjectElemOnPath, Odd, Even);

template <typename T>
class FormatsGetAtPathValue : public FormatsUtils<T> {};

TYPED_TEST_SUITE_P(FormatsGetAtPathValue);

TYPED_TEST_P(FormatsGetAtPathValue, Empty) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  Value result = origin.ExtractValue();
  const std::vector<std::string> path{};
  ASSERT_EQ(formats::common::GetAtPath(result, path), result);
}

TYPED_TEST_P(FormatsGetAtPathValue, NonObjectElemOnPath) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  Value origin_value = origin.ExtractValue();
  std::vector<std::string> path = {"key1", "key2"};
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  ASSERT_THROW(formats::common::GetAtPath(origin_value, std::move(path)),
               typename TestFixture::Exception);
}

TYPED_TEST_P(FormatsGetAtPathValue, One) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"]["key2"]["key3"] = 1;
  Value result = origin.ExtractValue();
  const std::vector<std::string> path{"key1"};
  ASSERT_EQ(formats::common::GetAtPath(result, path), result["key1"]);
}

TYPED_TEST_P(FormatsGetAtPathValue, Odd) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder builder;
  ValueBuilder new_elem = 2;
  builder["key1"]["key2"]["key3"] = new_elem;
  Value result = builder.ExtractValue();
  const std::vector<std::string> path{"key1", "key2", "key3"};
  ASSERT_EQ(formats::common::GetAtPath(result, path), new_elem.ExtractValue());
}

TYPED_TEST_P(FormatsGetAtPathValue, Even) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder builder;
  builder["key1"]["key2"]["key3"] = 1;
  Value result = builder.ExtractValue();
  const std::vector<std::string> path{"key1", "key2"};
  ASSERT_EQ(formats::common::GetAtPath(result, path), result["key1"]["key2"]);
}

REGISTER_TYPED_TEST_SUITE_P(FormatsGetAtPathValue, Empty, One, Odd, Even,
                            NonObjectElemOnPath);

template <typename T>
class FormatsSetAtPath : public FormatsUtils<T> {};

TYPED_TEST_SUITE_P(FormatsSetAtPath);

TYPED_TEST_P(FormatsSetAtPath, Empty) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path;
  ValueBuilder origin_test_empty;
  formats::common::SetAtPath(origin_test_empty, std::move(path), new_elem);
  ASSERT_EQ(origin_test_empty.ExtractValue(), new_elem);
}

TYPED_TEST_P(FormatsSetAtPath, NonObjectElemOnPath) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"] = "2";
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  ASSERT_THROW(formats::common::SetAtPath(origin, std::move(path), new_elem),
               typename TestFixture::Exception);
}

TYPED_TEST_P(FormatsSetAtPath, One) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"], ValueBuilder(2).ExtractValue());
}

TYPED_TEST_P(FormatsSetAtPath, Odd) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1", "key2", "key3"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"]["key2"]["key3"], ValueBuilder(2).ExtractValue());
}

TYPED_TEST_P(FormatsSetAtPath, Even) {
  using Value = TypeParam;
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  const Value new_elem = ValueBuilder(2).ExtractValue();
  std::vector<std::string> path{"key1", "key2"};
  formats::common::SetAtPath(origin, std::move(path), new_elem);
  const Value result = origin.ExtractValue();
  ASSERT_EQ(result["key1"]["key2"], ValueBuilder(2).ExtractValue());
}

REGISTER_TYPED_TEST_SUITE_P(FormatsSetAtPath, Empty, One, Odd, Even,
                            NonObjectElemOnPath);

template <typename T>
class FormatsRemoveAtPath : public FormatsUtils<T> {};

TYPED_TEST_SUITE_P(FormatsRemoveAtPath);

TYPED_TEST_P(FormatsRemoveAtPath, Empty) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path;
  origin["key1"] = 1;
  formats::common::RemoveAtPath(origin, std::move(path));
  ASSERT_TRUE(origin.IsEmpty());
}

TYPED_TEST_P(FormatsRemoveAtPath, One) {
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

TYPED_TEST_P(FormatsRemoveAtPath, Odd) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"]["key3"] = "2";
  formats::common::RemoveAtPath(origin, std::move(path));
  const auto result = origin.ExtractValue();
  ASSERT_TRUE(result["key1"].HasMember("key2"));
  ASSERT_FALSE(result["key1"]["key2"].HasMember("key3"));
}

TYPED_TEST_P(FormatsRemoveAtPath, Even) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2"};
  origin["key1"]["key2"] = 1;
  formats::common::RemoveAtPath(origin, std::move(path));
  const auto result = origin.ExtractValue();
  ASSERT_TRUE(result.HasMember("key1"));
  ASSERT_FALSE(result["key1"].HasMember("key2"));
}

TYPED_TEST_P(FormatsRemoveAtPath, NonObjectElemOnPath) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  std::vector<std::string> path{"key1", "key2", "key3"};
  origin["key1"]["key2"] = "2";
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  ASSERT_THROW(formats::common::RemoveAtPath(origin, std::move(path)),
               typename TestFixture::Exception);
}

REGISTER_TYPED_TEST_SUITE_P(FormatsRemoveAtPath, Empty, One, Odd, Even,
                            NonObjectElemOnPath);

template <typename T>
class FormatsTypeChecks : public FormatsUtils<T> {};

TYPED_TEST_SUITE_P(FormatsTypeChecks);

TYPED_TEST_P(FormatsTypeChecks, Empty) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder builder{formats::common::Type::kObject};
  ASSERT_TRUE(builder.IsEmpty());
  builder["key1"] = 1;
  ASSERT_FALSE(builder.IsEmpty());
}

TYPED_TEST_P(FormatsTypeChecks, Missing) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder builder{formats::common::Type::kObject};
  builder["key1"] = 1;
}

TYPED_TEST_P(FormatsTypeChecks, BasicTypes) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  origin["key1"] = 1;
  origin["key2"] = "sample";
  origin["key3"] = true;
  origin["key4"] = 1.23;

  ASSERT_TRUE(origin["key1"].IsInt());
  ASSERT_FALSE(origin["key2"].IsInt());

  ASSERT_TRUE(origin["key2"].IsString());

  ASSERT_TRUE(origin["key3"].IsBool());

  ASSERT_TRUE(origin["key4"].IsDouble());
  ASSERT_FALSE(origin["key2"].IsDouble());
}

TYPED_TEST_P(FormatsTypeChecks, Null) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder origin;
  ASSERT_TRUE(origin.IsNull());
  origin = 1;
  ASSERT_FALSE(origin.IsNull());
}

REGISTER_TYPED_TEST_SUITE_P(FormatsTypeChecks, Empty, Missing, BasicTypes,
                            Null);

USERVER_NAMESPACE_END
