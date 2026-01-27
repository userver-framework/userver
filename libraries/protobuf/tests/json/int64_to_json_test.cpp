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

constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();  // 9223372036854775807
constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();  // -9223372036854775808

struct Int64ToJsonSuccessTestParam {
    Int64MessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const Int64ToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format(
        "{{ input = {{.field1={}, .field2={}, .field3={}}} }}",
        param.input.field1,
        param.input.field2,
        param.input.field3
    );
}

class Int64ToJsonSuccessTest : public ::testing::TestWithParam<Int64ToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    Int64ToJsonSuccessTest,
    ::testing::Values(
        Int64ToJsonSuccessTestParam{Int64MessageData{0, 0, 0}, R"({})"},
        Int64ToJsonSuccessTestParam{
            Int64MessageData{0, 0, 0},
            R"({"field1":"0","field2":"0","field3":"0"})",
            {.always_print_fields_with_no_presence = true}
        },
        Int64ToJsonSuccessTestParam{Int64MessageData{1, 2, 3}, R"({"field1":"1","field2":"2","field3":"3"})"},
        Int64ToJsonSuccessTestParam{Int64MessageData{-1, -2, -3}, R"({"field1":"-1","field2":"-2","field3":"-3"})"},
        Int64ToJsonSuccessTestParam{
            Int64MessageData{kMax, kMax, kMax},
            R"({"field1":"9223372036854775807","field2":"9223372036854775807","field3":"9223372036854775807"})"
        },
        Int64ToJsonSuccessTestParam{
            Int64MessageData{kMin, kMin, kMin},
            R"({"field1":"-9223372036854775808","field2":"-9223372036854775808","field3":"-9223372036854775808"})"
        }
    )
);

TEST_P(Int64ToJsonSuccessTest, Test) {
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
