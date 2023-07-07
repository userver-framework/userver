#include <gtest/gtest.h>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <formats/common/conversion_test.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct Conversion<formats::json::Value> : public ::testing::Test {
  using ValueBuilder = formats::json::ValueBuilder;
  using Value = formats::json::Value;
  using Exception = formats::json::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Conversion, formats::json::Value);

USERVER_NAMESPACE_END
