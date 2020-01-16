#include <gtest/gtest.h>

#include <formats/yaml/exception.hpp>
#include <formats/yaml/value_builder.hpp>

#include <formats/common/value_builder_test.hpp>

template <>
struct Instantiation<formats::yaml::ValueBuilder> : public ::testing::Test {
  using ValueBuilder = formats::yaml::ValueBuilder;

  using Exception = formats::yaml::Exception;
};

INSTANTIATE_TYPED_TEST_CASE_P(FormatsYaml, Instantiation,
                              formats::yaml::ValueBuilder);
