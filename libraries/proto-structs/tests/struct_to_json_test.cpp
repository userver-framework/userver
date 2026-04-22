#include <gtest/gtest.h>

#include <userver/proto-structs/json.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/log_capture_fixture.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

namespace {

structs::Scalar MakeStruct() {
    return structs::Scalar{
        .f1 = true,
        .f2 = std::numeric_limits<int32_t>::min(),
        .f3 = std::numeric_limits<uint32_t>::max(),
        .f4 = std::numeric_limits<int64_t>::min(),
        .f5 = std::numeric_limits<uint64_t>::max(),
        .f6 = static_cast<float>(1.5),
        .f7 = -2.5,
        .f8 = "test1",
        .f9 = "test2",
        .f10 = structs::TestEnum::kValue1,
        .f11 = 987
    };
}

formats::json::Value MakeJson() {
    auto expected_builder = formats::json::ValueBuilder{formats::common::Type::kObject};
    expected_builder["f1"] = true;
    expected_builder["f2"] = std::numeric_limits<int32_t>::min();
    expected_builder["f3"] = std::numeric_limits<uint32_t>::max();
    // 64 bits are too large for JSON numbers, so we convert to string
    expected_builder["f4"] = std::to_string(std::numeric_limits<int64_t>::min());
    expected_builder["f5"] = std::to_string(std::numeric_limits<uint64_t>::max());
    expected_builder["f6"] = static_cast<float>(1.5);
    expected_builder["f7"] = -2.5;
    expected_builder["f8"] = "test1";
    expected_builder["f9"] = "dGVzdDI=";  // base64 encoded "test2"
    expected_builder["f10"] = "TEST_ENUM_VALUE1";
    expected_builder["f11"] = 987;
    return expected_builder.ExtractValue();
}

}  // namespace

TEST(StructToJson, JsonValue) {
    formats::json::Value json;
    UASSERT_NO_THROW(json = StructToJson(MakeStruct(), {}));
    ASSERT_EQ(json, MakeJson());
}

TEST(StructToJson, JsonString) {
    std::string json_string;
    UASSERT_NO_THROW(json_string = StructToJsonString(MakeStruct(), {}));
    ASSERT_EQ(formats::json::FromString(json_string), MakeJson());
}

TEST(StructToJson, Serialize) {
    formats::json::Value json;
    UASSERT_NO_THROW(json = formats::json::ValueBuilder{MakeStruct()}.ExtractValue());
    ASSERT_EQ(json, MakeJson());
}

using Logging = utest::LogCaptureFixture<>;

TEST_F(Logging, LogStruct) {
    const auto obj_string = GetLogCapture().ToStringViaLogging(MakeStruct());
    EXPECT_EQ(obj_string, ToString(MakeJson()));
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
