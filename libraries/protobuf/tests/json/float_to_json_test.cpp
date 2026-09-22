#include <gtest/gtest.h>

#include <limits>
#include <ostream>

#include <fmt/format.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

constexpr float kMax = std::numeric_limits<float>::max();
constexpr float kMin = std::numeric_limits<float>::min();

struct FloatToJsonSuccessTestParam {
    FloatMessageData input = {};
    PrintOptions options = {};
};

void PrintTo(const FloatToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1={}}} }}", param.input.field1);
}

class FloatToJsonSuccessTest : public ::testing::TestWithParam<FloatToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    FloatToJsonSuccessTest,
    ::testing::Values(
        FloatToJsonSuccessTestParam{FloatMessageData{0.0}},
        FloatToJsonSuccessTestParam{FloatMessageData{0}, {.always_print_fields_with_no_presence = true}},
        FloatToJsonSuccessTestParam{FloatMessageData{1}},
        FloatToJsonSuccessTestParam{FloatMessageData{-1}},
        FloatToJsonSuccessTestParam{FloatMessageData{100.12357}},
        FloatToJsonSuccessTestParam{FloatMessageData{-100.12357}},
        FloatToJsonSuccessTestParam{FloatMessageData{kMax}},
        FloatToJsonSuccessTestParam{FloatMessageData{-kMax}},
        FloatToJsonSuccessTestParam{FloatMessageData{kMin}},
        FloatToJsonSuccessTestParam{FloatMessageData{-kMin}}
    )
);

TEST_P(FloatToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json;
    formats::json::Value sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    // We don't compare 'formats::json::Value' directly using 'operator==' to avoid running into
    // different implementations of float output reduction. After converting to target type values should be equal.
    EXPECT_LE(json.GetSize(), std::size_t{1});
    EXPECT_EQ(json.GetSize(), sample_json.GetSize());

    if (json.GetSize() == 0) {
        return;
    }

    EXPECT_EQ(json["field1"].As<float>(), param.input.field1);
    EXPECT_EQ(json["field1"].As<float>(), sample_json["field1"].As<float>());
}

TEST(FloatToJsonAdditionalTest, ShortestRepresentation) {
    const auto input = PrepareTestData(FloatMessageData{0.1f});

    EXPECT_EQ(MessageToJsonString(input, {}), R"({"field1":0.1})");
    EXPECT_EQ(MessageToDebugString(input, 1000), R"({"field1":0.1})");

    const auto json = MessageToJson(input, {});
    EXPECT_EQ(json["field1"].As<double>(), 0.1);
    EXPECT_EQ(formats::json::ToString(json), R"({"field1":0.1})");

    const auto sample_json = CreateSampleJson(input, {});
    EXPECT_EQ(json, sample_json);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
