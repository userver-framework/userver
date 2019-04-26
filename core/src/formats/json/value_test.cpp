#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/serialize_container.hpp>
#include <formats/json/value.hpp>

#include <formats/common/value_test.hpp>

template <>
struct Parsing<formats::json::Value> : public ::testing::Test {
  constexpr static auto FromString = formats::json::FromString;
  using ParseException = formats::json::Value::ParseException;
};

INSTANTIATE_TYPED_TEST_CASE_P(FormatsJson, Parsing, formats::json::Value);
