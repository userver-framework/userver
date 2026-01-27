#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct StringToJsonSuccessTestParam {
    StringMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const StringToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1='{}'}} }}", param.input.field1);
}

class StringToJsonSuccessTest : public ::testing::TestWithParam<StringToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    StringToJsonSuccessTest,
    ::testing::Values(
        StringToJsonSuccessTestParam{StringMessageData{""}, R"({})"},
        StringToJsonSuccessTestParam{
            StringMessageData{""},
            R"({"field1":""})",
            {.always_print_fields_with_no_presence = true}
        },
        StringToJsonSuccessTestParam{StringMessageData{"hello world"}, R"({"field1":"hello world"})"}
    )
);

TEST_P(StringToJsonSuccessTest, Test) {
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
