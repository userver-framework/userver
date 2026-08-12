#include <gtest/gtest.h>

#include <userver/proto-structs/json.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

namespace {

messages::Scalar MakeMessage() {
    messages::Scalar msg;
    msg.set_f1(true);
    msg.set_f2(std::numeric_limits<int32_t>::min());
    msg.set_f3(std::numeric_limits<uint32_t>::max());
    msg.set_f4(std::numeric_limits<int64_t>::min());
    msg.set_f5(std::numeric_limits<uint64_t>::max());
    msg.set_f6(static_cast<float>(1.5));
    msg.set_f7(-2.5);
    msg.set_f8("test1");
    msg.set_f9("test2");
    msg.set_f10(messages::TestEnum::TEST_ENUM_VALUE1);
    msg.set_f11(987);
    return msg;
}

formats::json::Value MakeJson() {
    auto builder = formats::json::ValueBuilder{formats::common::Type::kObject};
    builder["f1"] = true;
    builder["f2"] = std::numeric_limits<int32_t>::min();
    builder["f3"] = std::numeric_limits<uint32_t>::max();
    // 64 bits are too large for JSON numbers, so we convert to string
    builder["f4"] = std::to_string(std::numeric_limits<int64_t>::min());
    builder["f5"] = std::to_string(std::numeric_limits<uint64_t>::max());
    builder["f6"] = static_cast<float>(1.5);
    builder["f7"] = -2.5;
    builder["f8"] = "test1";
    builder["f9"] = "dGVzdDI=";  // base64 encoded "test2"
    builder["f10"] = "TEST_ENUM_VALUE1";
    builder["f11"] = 987;
    return builder.ExtractValue();
}

}  // namespace

TEST(JsonToStruct, JsonValue) {
    structs::Scalar result;
    UASSERT_NO_THROW(result = JsonToStruct<structs::Scalar>(MakeJson(), {}));
    structs::CheckScalarEqual(result, MakeMessage());
}

TEST(JsonToStruct, JsonString) {
    const auto json_string = formats::json::ToString(MakeJson());
    structs::Scalar result;
    UASSERT_NO_THROW(result = JsonStringToStruct<structs::Scalar>(json_string, {}));
    structs::CheckScalarEqual(result, MakeMessage());
}

TEST(JsonToStruct, Parse) {
    structs::Scalar result;
    UASSERT_NO_THROW(result = MakeJson().As<structs::Scalar>());
    structs::CheckScalarEqual(result, MakeMessage());
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
