#include <userver/formats/common/merge.hpp>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct MergeTestParams {
  std::string_view original_json;
  std::string_view patch_json;
  std::string_view result_json;
};

void CheckJsonMerge(MergeTestParams check) {
  formats::json::ValueBuilder original =
      formats::json::FromString(check.original_json);
  const formats::json::Value patch =
      formats::json::FromString(check.patch_json);
  const formats::json::Value expected_result =
      formats::json::FromString(check.result_json);
  formats::common::Merge(original, patch);
  ASSERT_EQ(original.ExtractValue(), expected_result);
}

}  // namespace

TEST(FormatsMerge, MissingOriginal) {
  CheckJsonMerge({R"({"a": 1})", R"({"b": 2})", R"({"a": 1,"b": 2})"});
  CheckJsonMerge(
      {R"({"z": 0})", R"({"a": {"b": 1} })", R"({"z": 0, "a": {"b": 1} })"});
}

TEST(FormatsMerge, IntOriginal) {
  CheckJsonMerge({R"({"a": 1})", R"({"a": 2})", R"({"a": 2})"});
  CheckJsonMerge({R"({"a": 1})", R"({"a": {"b": 2} })", R"({"a": {"b": 2} })"});
}

TEST(FormatsMerge, ObjectOrigIntPatch) {
  CheckJsonMerge({R"({"a": {"b": 2} })", R"({"a": 1})", R"({"a": 1})"});
  CheckJsonMerge({R"({"a": {"b": 1} })", R"({"a": {"c": 2} })",
                  R"({"a": {"b": 1, "c": 2} })"});
}

TEST(FormatsMerge, Array) {
  CheckJsonMerge({R"({"a": {"b": [1]} })", R"({"a": {"b": [2]} })",
                  R"({"a": {"b": [2]} })"});
}

TEST(FormatsMerge, Null) {
  CheckJsonMerge({"null", "null", "null"});
  CheckJsonMerge({R"({"b": 1})", "null", "null"});
  CheckJsonMerge({R"({"a": {"b": 1} })", "null", "null"});
  CheckJsonMerge({"null", R"({"b": 1})", R"({"b": 1})"});
  CheckJsonMerge({"null", R"({"a": {"b": 1} })", R"({"a": {"b": 1} })"});
}

TEST(FormatsMerge, Empty) {
  CheckJsonMerge({R"({"a": 1})", "{}", R"({"a": 1})"});
  CheckJsonMerge({"{}", R"({"a": 1})", R"({"a": 1})"});
  CheckJsonMerge({R"({"a": {"b": 2} })", "{}", R"({"a": {"b": 2} })"});
  CheckJsonMerge({"{}", R"({"a": {"b": 2} })", R"({"a": {"b": 2} })"});
}

USERVER_NAMESPACE_END
