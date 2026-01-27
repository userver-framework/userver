#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/encoding/hex.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct BytesToJsonSuccessTestParam {
    BytesMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const BytesToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1='{}'}} }}", utils::encoding::ToHex(param.input.field1));
}

class BytesToJsonSuccessTest : public ::testing::TestWithParam<BytesToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BytesToJsonSuccessTest,
    ::testing::Values(
        BytesToJsonSuccessTestParam{BytesMessageData{""}, R"({})"},
        BytesToJsonSuccessTestParam{
            BytesMessageData{""},
            R"({"field1":""})",
            {.always_print_fields_with_no_presence = true}
        },
        BytesToJsonSuccessTestParam{BytesMessageData{std::string("\xfb\xfb", 2)}, R"({"field1":"+/s="})"}
    )
);

TEST_P(BytesToJsonSuccessTest, Test) {
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
