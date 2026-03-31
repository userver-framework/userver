#include <gtest/gtest.h>

#include <userver/proto-structs/json.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(JsonToStruct, JsonValue) {
    messages::Scalar expected;
    expected.set_f1(true);
    expected.set_f2(std::numeric_limits<int32_t>::min());
    expected.set_f3(std::numeric_limits<uint32_t>::max());
    expected.set_f4(std::numeric_limits<int64_t>::min());
    expected.set_f5(std::numeric_limits<uint64_t>::max());
    expected.set_f6(static_cast<float>(1.5));
    expected.set_f7(-2.5);
    expected.set_f8("test1");
    expected.set_f9("test2");
    expected.set_f10(messages::TestEnum::TEST_ENUM_VALUE1);
    expected.set_f11(987);

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

    structs::Scalar result;
    UASSERT_NO_THROW(result = JsonToStruct<structs::Scalar>(builder.ExtractValue(), {}));
    structs::CheckScalarEqual(result, expected);
}

TEST(JsonToStruct, JsonString) {
    messages::Scalar expected;
    expected.set_f1(true);
    expected.set_f2(std::numeric_limits<int32_t>::min());
    expected.set_f3(std::numeric_limits<uint32_t>::max());
    expected.set_f4(std::numeric_limits<int64_t>::min());
    expected.set_f5(std::numeric_limits<uint64_t>::max());
    expected.set_f6(static_cast<float>(1.5));
    expected.set_f7(-2.5);
    expected.set_f8("test1");
    expected.set_f9("test2");
    expected.set_f10(messages::TestEnum::TEST_ENUM_VALUE1);
    expected.set_f11(987);

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
    const auto json_string = formats::json::ToString(builder.ExtractValue());

    structs::Scalar result;
    UASSERT_NO_THROW(result = JsonStringToStruct<structs::Scalar>(json_string, {}));
    structs::CheckScalarEqual(result, expected);
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
