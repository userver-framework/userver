#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct NullValueFromJsonSuccessTestParam {
    std::string input = {};
    NullValueMessageData expected_message = {};
    ParseOptions options = {};
};

void PrintTo(const NullValueFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class NullValueFromJsonSuccessTest : public ::testing::TestWithParam<NullValueFromJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    NullValueFromJsonSuccessTest,
    ::testing::Values(
        NullValueFromJsonSuccessTestParam{R"({})", NullValueMessageData{}},
        NullValueFromJsonSuccessTestParam{R"({"field1":null})", NullValueMessageData{}},
        NullValueFromJsonSuccessTestParam{R"({"field2":null})", NullValueMessageData{.field2 = kNullConst}},
        NullValueFromJsonSuccessTestParam{
            R"({"field1":null,"field2":null,"field3":[null,null],"field4":{"100":null,"200":null}})",
            NullValueMessageData{
                kNullConst,
                kNullConst,
                {kNullConst, kNullConst},
                {{100, kNullConst}, {200, kNullConst}}
            }
        }
    )
);

TEST_P(NullValueFromJsonSuccessTest, Test) {
    using Message = proto_json::messages::NullValueMessage;
    const auto& param = GetParam();

    Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
