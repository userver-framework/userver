#include <gtest/gtest.h>

#include <string>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct RepeatedToJsonSuccessTestParam {
    RepeatedMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

struct RepeatedToJsonFailureTestParam {
    RepeatedMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};
};

class RepeatedToJsonSuccessTest : public ::testing::TestWithParam<RepeatedToJsonSuccessTestParam> {};
class RepeatedToJsonFailureTest : public ::testing::TestWithParam<RepeatedToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    RepeatedToJsonSuccessTest,
    ::testing::Values(
        RepeatedToJsonSuccessTestParam{RepeatedMessageData{}, R"({})"},
        RepeatedToJsonSuccessTestParam{
            RepeatedMessageData{},
            R"({"field1":[],"field2":[],"field3":[]})",
            {.always_print_fields_with_no_presence = true}
        },
        RepeatedToJsonSuccessTestParam{RepeatedMessageData{.field1 = {1, 2, 3}}, R"({"field1":[1,2,3]})"},
        RepeatedToJsonSuccessTestParam{
            RepeatedMessageData{.field2 = {{true}, {false}, {true}, {true}}},
            R"({"field2":[{"field1":true},{},{"field1":true},{"field1":true}]})"
        },
        RepeatedToJsonSuccessTestParam{
            RepeatedMessageData{.field3 = {{.seconds = 123, .nanos = 987000000}}},
            R"({"field3":["123.987s"]})"
        },
        RepeatedToJsonSuccessTestParam{
            RepeatedMessageData{.field1 = {0}, .field2 = {{false}}, .field3 = {{.seconds = 0, .nanos = 0}}},
            R"({"field1":[0],"field2":[{}],"field3":["0s"]})"
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    RepeatedToJsonFailureTest,
    ::testing::Values(RepeatedToJsonFailureTestParam{
        RepeatedMessageData{
            .field3 = {{.seconds = 1, .nanos = 1}, {.seconds = 1, .nanos = -1}, {.seconds = 0, .nanos = 1}}
        },
        PrintErrorCode::kInvalidValue,
        "field3[1]"
    })
);

TEST_P(RepeatedToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(RepeatedToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);
    UEXPECT_THROW((void)CreateSampleJson(input, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
