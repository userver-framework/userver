#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>

#include <schemas/object_free_form.hpp>
#include <schemas/object_free_form_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

// A free-form object is any JSON object. A named one is a type of its own, so that parsing it checks for an object
static_assert(std::is_same_v<ns::ObjectFreeForm, chaotic::FreeFormObject>);
static_assert(std::is_base_of_v<formats::json::Value, ns::ObjectFreeForm>);
static_assert(std::is_same_v<decltype(ns::ObjectWithFreeForm::payload), formats::json::Value>);
static_assert(std::is_same_v<decltype(ns::ObjectWithFreeForm::ref), std::optional<ns::ObjectFreeForm>>);

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
    UEXPECT_THROW(CallSaxParser<ns::ObjectWithFreeForm>(R"({"payload":{},"ref":[]})"), formats::json::Exception);
}

TEST(FreeForm, Named) {
    const auto json = formats::json::FromString(R"({"a":[1,{"b":null}]})");
    const auto obj = json.As<ns::ObjectFreeForm>();
    EXPECT_EQ(obj, json);
    EXPECT_EQ(obj["a"][0].As<int>(), 1);
    EXPECT_EQ(CallSaxParser<ns::ObjectFreeForm>(ToString(json)), json);
    EXPECT_EQ(formats::json::ValueBuilder{obj}.ExtractValue(), json);
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(ns::ObjectFreeForm{}, formats::json::MakeObject());
    EXPECT_EQ(formats::json::ValueBuilder{ns::ObjectFreeForm{}}.ExtractValue(), formats::json::MakeObject());
}

TEST(FreeForm, NamedArray) {
    const auto json = formats::json::FromString(R"([{"first":[1,2]},{}])");
    for (const auto& objects :
         {json.As<ns::ObjectFreeFormArray>(), CallSaxParser<ns::ObjectFreeFormArray>(ToString(json))}) {
        ASSERT_EQ(objects.size(), 2);
        EXPECT_EQ(objects[0], json[0]);
        EXPECT_EQ(objects[1], formats::json::MakeObject());
    }
    UEXPECT_THROW(formats::json::FromString(R"([{},1])").As<ns::ObjectFreeFormArray>(), formats::json::Exception);
    UEXPECT_THROW(CallSaxParser<ns::ObjectFreeFormArray>(R"([{},1])"), formats::json::Exception);
}

TEST(FreeForm, NamedNotAnObject) {
    for (const auto* input : {"null", "true", "42", R"("text")", "[]"}) {
        UEXPECT_THROW(formats::json::FromString(input).As<ns::ObjectFreeForm>(), formats::json::Exception) << input;
    }

    chaotic::sax::Parser<ns::ObjectFreeForm> parser;
    for (const std::string_view input : {"null", "true", "42", R"("text")", "[]", "{", R"({"key":[1,})"}) {
        EXPECT_THROW((formats::json::parser::impl::ParseSingle(parser, input)), formats::json::parser::ParseError)
            << input;
        // The parser is reusable after an error
        const auto result = formats::json::parser::impl::ParseSingle(parser, R"({"ok":true})");
        EXPECT_EQ(result, formats::json::FromString(R"({"ok":true})"));
    }
}

USERVER_NAMESPACE_END
