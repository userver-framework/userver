#include <gtest/gtest.h>

#include <string>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct NullValueToJsonSuccessTestParam {
    NullValueMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

class NullValueToJsonSuccessTest : public ::testing::TestWithParam<NullValueToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    NullValueToJsonSuccessTest,
    ::testing::Values(
        NullValueToJsonSuccessTestParam{NullValueMessageData{}, R"({})"},
        NullValueToJsonSuccessTestParam{
            NullValueMessageData{},
            R"({"field1":null,"field3":[],"field4":{}})",
            {.always_print_fields_with_no_presence = true}
        },
        NullValueToJsonSuccessTestParam{NullValueMessageData{.field2 = kNullConst}, R"({"field2":null})"},
        NullValueToJsonSuccessTestParam{
            NullValueMessageData{
                .field2 = kNullConst,
                .field3 = {kNullConst, kNullConst},
                .field4 = {{1, kNullConst}, {2, kNullConst}}
            },
            R"({"field2":null,"field3":[null,null],"field4":{"1":null,"2":null}})"
        }
    )
);

TEST_P(NullValueToJsonSuccessTest, Test) {
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
