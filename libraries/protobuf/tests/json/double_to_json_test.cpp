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

constexpr double kMax = std::numeric_limits<double>::max();
constexpr double kMin = std::numeric_limits<double>::min();

struct DoubleToJsonSuccessTestParam {
    DoubleMessageData input = {};
    PrintOptions options = {};
};

void PrintTo(const DoubleToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1={}}} }}", param.input.field1);
}

class DoubleToJsonSuccessTest : public ::testing::TestWithParam<DoubleToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    DoubleToJsonSuccessTest,
    ::testing::Values(
        DoubleToJsonSuccessTestParam{DoubleMessageData{0.0}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{0}, {.always_print_fields_with_no_presence = true}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{1}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{-1}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{100.12357}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{-100.12357}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{kMax}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{-kMax}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{kMin}},
        DoubleToJsonSuccessTestParam{DoubleMessageData{-kMin}}
    )
);

TEST_P(DoubleToJsonSuccessTest, Test) {
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

    EXPECT_EQ(json["field1"].As<double>(), param.input.field1);
    EXPECT_EQ(json["field1"].As<double>(), sample_json["field1"].As<double>());
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
