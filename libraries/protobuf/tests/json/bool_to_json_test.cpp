#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct BoolToJsonSuccessTestParam {
    BoolMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const BoolToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1={}}} }}", param.input.field1);
}

class BoolToJsonSuccessTest : public ::testing::TestWithParam<BoolToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BoolToJsonSuccessTest,
    ::testing::Values(
        BoolToJsonSuccessTestParam{BoolMessageData{false}, R"({})"},
        BoolToJsonSuccessTestParam{
            BoolMessageData{false},
            R"({"field1":false})",
            {.always_print_fields_with_no_presence = true}
        },
        BoolToJsonSuccessTestParam{BoolMessageData{true}, R"({"field1":true})"}
    )
);

TEST_P(BoolToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
