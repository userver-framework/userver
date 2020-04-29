#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/value_builder.hpp>

#include <formats/common/value_builder_test.hpp>

template <>
struct Instantiation<formats::json::ValueBuilder> : public ::testing::Test {
  using ValueBuilder = formats::json::ValueBuilder;

  using Exception = formats::json::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Instantiation,
                               formats::json::ValueBuilder);
