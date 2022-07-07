#include <formats/common/utils_test.hpp>

#include <gtest/gtest-typed-test.h>

#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <>
class FormatsUtils<formats::yaml::Value> : public ::testing::Test {
 public:
  using Exception = formats::yaml::TypeMismatchException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(YamlGetAtPathValueBuilder,
                               FormatsGetAtPathValueBuilder,
                               formats::yaml::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(YamlGetAtPathValue, FormatsGetAtPathValue,
                               formats::yaml::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(YamlSetAtPath, FormatsSetAtPath,
                               formats::yaml::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(YamlRemoveAtPath, FormatsRemoveAtPath,
                               formats::yaml::Value);

INSTANTIATE_TYPED_TEST_SUITE_P(YamlTypeChecks, FormatsTypeChecks,
                               formats::yaml::Value);

USERVER_NAMESPACE_END
