#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct NestedToJsonSuccessTestParam {
    NestedMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const NestedToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format(
        "{{ input = {{.field1={}}} }}",
        param.input.field1 ? std::to_string(param.input.field1.value()) : "nullopt"
    );
}

class NestedToJsonSuccessTest : public ::testing::TestWithParam<NestedToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    NestedToJsonSuccessTest,
    ::testing::Values(
        NestedToJsonSuccessTestParam{NestedMessageData{std::nullopt}, R"({})"},
        NestedToJsonSuccessTestParam{
            NestedMessageData{std::nullopt},
            R"({})",
            {.always_print_fields_with_no_presence = true}
        },
        NestedToJsonSuccessTestParam{NestedMessageData{0}, R"({"parent":{}})"},
        NestedToJsonSuccessTestParam{
            NestedMessageData{0},
            R"({"parent":{"field1":0}})",
            {.always_print_fields_with_no_presence = true}
        },
        NestedToJsonSuccessTestParam{NestedMessageData{123}, R"({"parent":{"field1":123}})"}
    )
);

TEST_P(NestedToJsonSuccessTest, Test) {
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
