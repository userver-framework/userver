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

constexpr std::uint32_t kMax = std::numeric_limits<std::uint32_t>::max();  // 4294967295

struct UInt32ToJsonSuccessTestParam {
    UInt32MessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const UInt32ToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1={}, .field2={}}} }}", param.input.field1, param.input.field2);
}

class UInt32ToJsonTest : public ::testing::TestWithParam<UInt32ToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    UInt32ToJsonTest,
    ::testing::Values(
        UInt32ToJsonSuccessTestParam{UInt32MessageData{0, 0}, R"({})"},
        UInt32ToJsonSuccessTestParam{
            UInt32MessageData{0, 0},
            R"({"field1":0,"field2":0})",
            {.always_print_fields_with_no_presence = true}
        },
        UInt32ToJsonSuccessTestParam{UInt32MessageData{1, 2}, R"({"field1":1,"field2":2})"},
        UInt32ToJsonSuccessTestParam{UInt32MessageData{kMax, kMax}, R"({"field1":4294967295,"field2":4294967295})"}
    )
);

TEST_P(UInt32ToJsonTest, Test) {
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
