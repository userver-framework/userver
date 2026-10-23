#include <userver/formats/json/serialize.hpp>
#include <userver/utest/assert_macros.hpp>

#include <schemas/object_free_form.hpp>
#include <schemas/object_free_form_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

// A free-form object is any JSON object
static_assert(std::is_same_v<ns::ObjectFreeForm, formats::json::Value>);
static_assert(std::is_same_v<decltype(ns::ObjectWithFreeForm::payload), formats::json::Value>);
static_assert(std::is_same_v<decltype(ns::ObjectWithFreeForm::ref), std::optional<formats::json::Value>>);

TEST(FreeForm, Parse) {
    const auto json = formats::json::FromString(R"({"payload":{"a":[1,{"b":null}]},"ref":{"c":"d"}})");
    const auto obj = json.As<ns::ObjectWithFreeForm>();
    EXPECT_EQ(obj.payload, json["payload"]);
    EXPECT_EQ(obj.ref, json["ref"]);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(FreeForm, ParseSax) {
    const auto json = formats::json::FromString(R"({"payload":{"a":[1,{"b":null}]},"ref":{"c":"d"}})");
    const auto obj = CallSaxParser<ns::ObjectWithFreeForm>(ToString(json));
    EXPECT_EQ(obj.payload, json["payload"]);
    EXPECT_EQ(obj.ref, json["ref"]);
}

TEST(FreeForm, NotAnObject) {
    for (const auto* payload : {"[]", "1", "\"x\"", "null"}) {
        const auto json = formats::json::FromString(fmt::format(R"({{"payload":{}}})", payload));
        UEXPECT_THROW(json.As<ns::ObjectWithFreeForm>(), formats::json::Exception) << payload;
        UEXPECT_THROW(CallSaxParser<ns::ObjectWithFreeForm>(ToString(json)), formats::json::Exception) << payload;
    }
    UEXPECT_THROW(
        formats::json::FromString(R"({"payload":{},"ref":[]})").As<ns::ObjectWithFreeForm>(),
        formats::json::Exception
    );
}

USERVER_NAMESPACE_END
