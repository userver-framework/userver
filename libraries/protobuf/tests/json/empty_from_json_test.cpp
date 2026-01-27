#include <gtest/gtest.h>

#include <string>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

class EmptyFromJsonFailureTest : public ::testing::TestWithParam<std::string> {};
INSTANTIATE_TEST_SUITE_P(, EmptyFromJsonFailureTest, ::testing::Values("[]", "10", "true", "\"hello\""));

TEST_P(EmptyFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::EmptyMessage message;
    formats::json::Value input = PrepareJsonTestData(param);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::EmptyMessage>(input),
        ParseErrorCode::kInvalidType,
        "/"
    );
    UEXPECT_THROW(InitSampleMessage(param, message), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
