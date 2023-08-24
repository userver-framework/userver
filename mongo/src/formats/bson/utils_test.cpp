#include <formats/common/utils_test.hpp>

#include <gtest/gtest-typed-test.h>

#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <>
class FormatsUtils<formats::bson::Value> : public ::testing::Test {
 public:
  using Exception = formats::bson::TypeMismatchException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(BsonGetAtPathValueBuilder,
                               FormatsGetAtPathValueBuilder,
                               formats::bson::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(BsonGetAtPathValue, FormatsGetAtPathValue,
                               formats::bson::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(BsonSetAtPath, FormatsSetAtPath,
                               formats::bson::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(BsonRemoveAtPath, FormatsRemoveAtPath,
                               formats::bson::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(BsonTypeChecks, FormatsTypeChecks,
                               formats::bson::Value);

USERVER_NAMESPACE_END
