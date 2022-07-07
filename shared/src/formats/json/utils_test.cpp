#include <formats/common/utils_test.hpp>

#include <gtest/gtest-typed-test.h>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <>
class FormatsUtils<formats::json::Value> : public ::testing::Test {
 public:
  using Exception = formats::json::TypeMismatchException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(JsonGetAtPathValueBuilder,
                               FormatsGetAtPathValueBuilder,
                               formats::json::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(JsonGetAtPathValue, FormatsGetAtPathValue,
                               formats::json::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(JsonSetAtPath, FormatsSetAtPath,
                               formats::json::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(JsonRemoveAtPath, FormatsRemoveAtPath,
                               formats::json::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(JsonTypeChecks, FormatsTypeChecks,
                               formats::json::Value);

USERVER_NAMESPACE_END
