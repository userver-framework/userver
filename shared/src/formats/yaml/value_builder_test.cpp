#include <gtest/gtest.h>

#include <formats/yaml/exception.hpp>
#include <formats/yaml/value_builder.hpp>

#include <formats/common/value_builder_test.hpp>

template <>
struct InstantiationDeathTest<formats::yaml::ValueBuilder>
    : public ::testing::Test {
  using ValueBuilder = formats::yaml::ValueBuilder;

  using Exception = formats::yaml::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, InstantiationDeathTest,
                               formats::yaml::ValueBuilder);
INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, CommonValueBuilderTests,
                               formats::yaml::ValueBuilder);
