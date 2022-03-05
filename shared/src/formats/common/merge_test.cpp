#include <userver/formats/common/merge.hpp>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct MergeTestParams {
  std::string original_json;
  std::string patch_json;
  std::string result_json;
};

template <typename Value, auto FromString>
void CheckJsonMerge(MergeTestParams check) {
  typename Value::Builder original = FromString(check.original_json);
  const Value patch = FromString(check.patch_json);
  const Value expected_result = FromString(check.result_json);
  formats::common::Merge(original, patch);
  ASSERT_EQ(original.ExtractValue(), expected_result);
}

using TestTypes = ::testing::Types<formats::json::Value, formats::yaml::Value>;

template <typename Value>
class FormatsMerge;

template <>
class FormatsMerge<formats::json::Value> : public ::testing::Test {
 public:
  constexpr static auto CheckMerge =
      CheckJsonMerge<formats::json::Value, formats::json::FromString>;
};

template <>
class FormatsMerge<formats::yaml::Value> : public ::testing::Test {
 public:
  constexpr static auto CheckMerge =
      CheckJsonMerge<formats::yaml::Value, formats::yaml::FromString>;
};

}  // namespace

TYPED_TEST_SUITE(FormatsMerge, TestTypes);

TYPED_TEST(FormatsMerge, MissingOrigin) {
  TestFixture::CheckMerge({R"({"a": 1})", R"({"b": 2})", R"({"a": 1,"b": 2})"});
  TestFixture::CheckMerge(
      {R"({"z": 0})", R"({"a": {"b": 1} })", R"({"z": 0, "a": {"b": 1} })"});
}

TYPED_TEST(FormatsMerge, IntOriginal) {
  TestFixture::CheckMerge({R"({"a": 1})", R"({"a": 2})", R"({"a": 2})"});
  TestFixture::CheckMerge(
      {R"({"a": 1})", R"({"a": {"b": 2} })", R"({"a": {"b": 2} })"});
}

TYPED_TEST(FormatsMerge, ObjectOrigIntPatch) {
  TestFixture::CheckMerge(
      {R"({"a": {"b": 2} })", R"({"a": 1})", R"({"a": 1})"});
  TestFixture::CheckMerge({R"({"a": {"b": 1} })", R"({"a": {"c": 2} })",
                           R"({"a": {"b": 1, "c": 2} })"});
}

TYPED_TEST(FormatsMerge, Array) {
  TestFixture::CheckMerge({R"({"a": {"b": [1]} })", R"({"a": {"b": [2]} })",
                           R"({"a": {"b": [2]} })"});
}

TYPED_TEST(FormatsMerge, Null) {
  TestFixture::CheckMerge({"null", "null", "null"});
  TestFixture::CheckMerge({R"({"b": 1})", "null", "null"});
  TestFixture::CheckMerge({R"({"a": {"b": 1} })", "null", "null"});
  TestFixture::CheckMerge({"null", R"({"b": 1})", R"({"b": 1})"});
  TestFixture::CheckMerge(
      {"null", R"({"a": {"b": 1} })", R"({"a": {"b": 1} })"});
}

TYPED_TEST(FormatsMerge, Empty) {
  TestFixture::CheckMerge({R"({"a": 1})", "{}", R"({"a": 1})"});
  TestFixture::CheckMerge({"{}", R"({"a": 1})", R"({"a": 1})"});
  TestFixture::CheckMerge({R"({"a": {"b": 2} })", "{}", R"({"a": {"b": 2} })"});
  TestFixture::CheckMerge({"{}", R"({"a": {"b": 2} })", R"({"a": {"b": 2} })"});
}

USERVER_NAMESPACE_END
