#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <gmock/gmock.h>

#include <userver/utest/log_capture_fixture.hpp>

#include <schemas/object_single_field.hpp>

USERVER_NAMESPACE_BEGIN

using Logging = utest::LogCaptureFixture<>;

TEST_F(Logging, Object) {
  auto json = formats::json::MakeObject(
      "integer", 1, "boolean", true, "number", 1.1, "string", "foo",
      "string-enum", "1", "object", formats::json::MakeObject(), "array",
      formats::json::MakeArray(1));
  auto obj = json.As<ns::ObjectTypes>();

  LOG_INFO() << obj;
  ASSERT_THAT(
      ExtractRawLog(),
      testing::HasSubstr(
          "text={\"boolean\":true,\"integer\":1,\"number\":1.1,\"string\":"
          "\"foo\",\"object\":{},\"array\":[1],\"string-enum\":\"1\"}\n"));

  LOG_INFO() << obj.string_enum;
  ASSERT_THAT(ExtractRawLog(), testing::HasSubstr("\ttext=1\n"));
}

USERVER_NAMESPACE_END
