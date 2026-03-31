#include <gtest/gtest.h>

#include <userver/proto-structs/json.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(StructToJson, JsonValue) {
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

    structs::Scalar obj{
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

    formats::json::Value json;
    UASSERT_NO_THROW(json = StructToJson(obj, {}));
    ASSERT_EQ(json, expected_builder.ExtractValue());
}

TEST(StructToJson, JsonString) {
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

    structs::Scalar obj{
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

    std::string json_string;
    UASSERT_NO_THROW(json_string = StructToJsonString(obj, {}));
    ASSERT_EQ(formats::json::FromString(json_string), expected_builder.ExtractValue());
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
