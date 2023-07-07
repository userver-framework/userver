#include <userver/formats/common/merge.hpp>

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <typename Value, auto ObjectFromJsonString>
void CheckJsonMerge(const std::string& original_json,
                    const std::string& patch_json,
                    const std::string& result_json) {
  typename Value::Builder original = ObjectFromJsonString(original_json);
  const Value patch = ObjectFromJsonString(patch_json);
  const Value expected_result = ObjectFromJsonString(result_json);
  formats::common::Merge(original, patch);
  ASSERT_EQ(original.ExtractValue(), expected_result);
}

template <typename Value>
class FormatsMerge;

TYPED_TEST_SUITE_P(FormatsMerge);

TYPED_TEST_P(FormatsMerge, MissingOrigin) {
  TestFixture::CheckMerge(R"({"a": 1})", R"({"b": 2})", R"({"a": 1,"b": 2})");
  TestFixture::CheckMerge(R"({"z": 0})", R"({"a": {"b": 1} })",
                          R"({"z": 0, "a": {"b": 1} })");
}

TYPED_TEST_P(FormatsMerge, IntOriginal) {
  TestFixture::CheckMerge(R"({"a": 1})", R"({"a": 2})", R"({"a": 2})");
  TestFixture::CheckMerge(R"({"a": 1})", R"({"a": {"b": 2} })",
                          R"({"a": {"b": 2} })");
}

TYPED_TEST_P(FormatsMerge, ObjectOrigIntPatch) {
  TestFixture::CheckMerge(R"({"a": {"b": 2} })", R"({"a": 1})", R"({"a": 1})");
  TestFixture::CheckMerge(R"({"a": {"b": 1} })", R"({"a": {"c": 2} })",
                          R"({"a": {"b": 1, "c": 2} })");
}

TYPED_TEST_P(FormatsMerge, Array) {
  TestFixture::CheckMerge(R"({"a": {"b": [1]} })", R"({"a": {"b": [2]} })",
                          R"({"a": {"b": [2]} })");
}

TYPED_TEST_P(FormatsMerge, Null) {
  TestFixture::CheckMerge(R"({"a": null})", R"({"a": null})", R"({"a": null})");
  TestFixture::CheckMerge(R"({"a": {"b": 1}})", R"({"a": null})",
                          R"({"a": null})");
  TestFixture::CheckMerge(R"({"a": {"b": 1} })", R"({"a": null})",
                          R"({"a": null})");
  TestFixture::CheckMerge(R"({"a": null})", R"({"a": {"b": 1}})",
                          R"({"a": {"b": 1}})");
  TestFixture::CheckMerge(R"({"a": null})", R"({"a": {"b": 1}})",
                          R"({"a": {"b": 1}})");
}

TYPED_TEST_P(FormatsMerge, Empty) {
  TestFixture::CheckMerge(R"({"a": 1})", "{}", R"({"a": 1})");
  TestFixture::CheckMerge("{}", R"({"a": 1})", R"({"a": 1})");
  TestFixture::CheckMerge(R"({"a": {"b": 2} })", "{}", R"({"a": {"b": 2} })");
  TestFixture::CheckMerge("{}", R"({"a": {"b": 2} })", R"({"a": {"b": 2} })");
}

REGISTER_TYPED_TEST_SUITE_P(FormatsMerge, MissingOrigin, IntOriginal,
                            ObjectOrigIntPatch, Array, Null, Empty);

USERVER_NAMESPACE_END
