#include <gtest/gtest.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

TEST(EmptyToJsonSuccessTest, Test) {
    proto_json::messages::EmptyMessage input;
    formats::json::Value json;
    formats::json::Value expected_json;
    formats::json::Value sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, {})));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData("{}")));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
