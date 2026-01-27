#include <gtest/gtest.h>

#include <limits>
#include <ostream>
#include <string>

#include <fmt/format.h>

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
    formats::json::Value json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    // Can't compare 'formats::json::Value' using 'operator==' because protobuf json writer outputs
    // distinct number of fractional digits than usever's one. Still after converting to target
    // type values should be equal.

    EXPECT_LE(json.GetSize(), std::size_t{1});
    EXPECT_EQ(json.GetSize(), sample_json.GetSize());

    if (json.GetSize() == 0) {
        return;
    }

    EXPECT_EQ(json["field1"].As<float>(), param.input.field1);
    EXPECT_EQ(json["field1"].As<float>(), sample_json["field1"].As<float>());
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
