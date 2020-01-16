#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>

#include <formats/common/serialize_test.hpp>
#include <formats/json/value_builder.hpp>

template <>
struct Serialization<formats::json::Value> : public ::testing::Test {
  constexpr static const char* kDoc = "{\"key1\":1,\"key2\":\"val\"}";

  using ValueBuilder = formats::json::ValueBuilder;
  using Value = formats::json::Value;
  using Type = formats::json::Type;

  using ParseException = formats::json::ParseException;
  using TypeMismatchException = formats::json::TypeMismatchException;
  using OutOfBoundsException = formats::json::OutOfBoundsException;
  using MemberMissingException = formats::json::MemberMissingException;
  using BadStreamException = formats::json::BadStreamException;

  constexpr static auto FromString = formats::json::FromString;
  constexpr static auto FromStream = formats::json::FromStream;
};

INSTANTIATE_TYPED_TEST_CASE_P(FormatsJson, Serialization, formats::json::Value);
