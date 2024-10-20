#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <gmock/gmock.h>

#include <userver/utest/log_capture_fixture.hpp>

#include <schemas/object_single_field.hpp>

USERVER_NAMESPACE_BEGIN

using Logging = utest::LogCaptureFixture<>;

TEST_F(Logging, Object) {
    auto json = formats::json::MakeObject(
        "integer",
        1,
        "boolean",
        true,
        "number",
        1.5,
        "string",
        "foo",
        "string-enum",
        "1",
        "object",
        formats::json::MakeObject(),
        "array",
        formats::json::MakeArray(1)
    );
    auto obj = json.As<ns::ObjectTypes>();

    const auto obj_string = GetLogCapture().ToStringViaLogging(obj);
    EXPECT_EQ(
        obj_string,
        R"({"boolean":true,"integer":1,"number":1.5,"string":"foo",)"
        R"("object":{},"array":[1],"string-enum":"1"})"
    );

    const auto enum_string = GetLogCapture().ToStringViaLogging(obj.string_enum);
    ASSERT_EQ(enum_string, "1");
}

USERVER_NAMESPACE_END
