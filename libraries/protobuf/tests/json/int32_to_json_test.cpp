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

constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();  // 2147483647
constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();  // -2147483648

struct Int32ToJsonSuccessTestParam {
    Int32MessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const Int32ToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format(
        "{{ input = {{.field1={}, .field2={}, .field3={}}} }}",
        param.input.field1,
        param.input.field2,
        param.input.field3
    );
}

class Int32ToJsonTest : public ::testing::TestWithParam<Int32ToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    Int32ToJsonTest,
    ::testing::Values(
        Int32ToJsonSuccessTestParam{Int32MessageData{0, 0, 0}, R"({})"},
        Int32ToJsonSuccessTestParam{
            Int32MessageData{0, 0, 0},
            R"({"field1":0,"field2":0,"field3":0})",
            {.always_print_fields_with_no_presence = true}
        },
        Int32ToJsonSuccessTestParam{Int32MessageData{1, 2, 3}, R"({"field1":1,"field2":2,"field3":3})"},
        Int32ToJsonSuccessTestParam{Int32MessageData{-1, -2, -3}, R"({"field1":-1,"field2":-2,"field3":-3})"},
        Int32ToJsonSuccessTestParam{
            Int32MessageData{kMax, kMax, kMax},
            R"({"field1":2147483647,"field2":2147483647,"field3":2147483647})"
        },
        Int32ToJsonSuccessTestParam{
            Int32MessageData{kMin, kMin, kMin},
            R"({"field1":-2147483648,"field2":-2147483648,"field3":-2147483648})"
        }
    )
);

TEST_P(Int32ToJsonTest, Test) {
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
