#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>

#include <schemas/array_default.hpp>
#include <schemas/array_default_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using Strings = std::vector<std::string>;

const formats::json::Value kOnlyRequired = formats::json::MakeObject("required_items", formats::json::MakeArray());

void ExpectDeclaredDefaults(const ns::ArrayDefaults& obj) {
    EXPECT_EQ(obj.optional_items, std::nullopt);
    EXPECT_EQ(obj.null_default, std::nullopt);
    EXPECT_EQ(obj.empty_default, Strings{});
    EXPECT_EQ(obj.string_default, (Strings{"sports", "quoted \"news\"\n"}));
    EXPECT_EQ(obj.integer_default, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(obj.number_default, (std::vector<double>{1.0, 2.5}));
    EXPECT_EQ(obj.boolean_default, (std::vector<bool>{true, false}));
    EXPECT_EQ(obj.enum_default, std::vector<ns::Unit>{ns::Unit::kImperial});
    EXPECT_EQ(obj.set_default, (std::set<std::string>{"a", "b"}));
    EXPECT_EQ(obj.ref_default, Strings{"tagged"});
    EXPECT_EQ(*obj.indirect_default, Strings{"tagged"});
    EXPECT_EQ(obj.custom_empty_default.s, Strings{});
    EXPECT_EQ(obj.custom_default.s, Strings{"custom"});
}

}  // namespace

TEST(ArrayDefault, MemberTypes) {
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::required_items), Strings>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::optional_items), std::optional<Strings>>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::null_default), std::optional<Strings>>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::empty_default), Strings>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::string_default), Strings>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::set_default), std::set<std::string>>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::ref_default), ns::Tags>);
    static_assert(std::is_same_v<decltype(ns::ArrayDefaults::custom_default), my::CustomArray<std::string>>);
}

TEST(ArrayDefault, MemberInitializers) {
    const ns::ArrayDefaults obj;
    EXPECT_EQ(obj.required_items, Strings{});
    ExpectDeclaredDefaults(obj);
}

TEST(ArrayDefault, AbsentKeysTakeDefaults) { ExpectDeclaredDefaults(kOnlyRequired.As<ns::ArrayDefaults>()); }

TEST(ArrayDefault, AbsentKeysTakeDefaultsSax) {
    ExpectDeclaredDefaults(CallSaxParser<ns::ArrayDefaults>(formats::json::ToString(kOnlyRequired)));
}

TEST(ArrayDefault, PresentKeysOverrideDefaults) {
    const auto json = formats::json::MakeObject(
        "required_items",
        formats::json::MakeArray(),
        "empty_default",
        formats::json::MakeArray("x"),
        "string_default",
        formats::json::MakeArray(),
        "custom_default",
        formats::json::MakeArray("y", "z")
    );

    for (const auto& obj :
         {json.As<ns::ArrayDefaults>(), CallSaxParser<ns::ArrayDefaults>(formats::json::ToString(json))})
    {
        EXPECT_EQ(obj.empty_default, Strings{"x"});
        EXPECT_EQ(obj.string_default, Strings{});
        EXPECT_EQ(obj.custom_default.s, (Strings{"y", "z"}));
    }
}

TEST(ArrayDefault, AbsentKeysTakeDefaultsInEveryArrayElement) {
    const auto overriding = formats::json::MakeObject(
        "required_items",
        formats::json::MakeArray(),
        "string_default",
        formats::json::MakeArray("x")
    );
    const auto json = formats::json::MakeObject("entries", formats::json::MakeArray(overriding, kOnlyRequired));

    for (const auto& holder :
         {json.As<ns::ArrayDefaultsHolder>(), CallSaxParser<ns::ArrayDefaultsHolder>(formats::json::ToString(json))})
    {
        ASSERT_EQ(holder.entries.size(), 2);
        EXPECT_EQ(holder.entries[0].string_default, Strings{"x"});
        ExpectDeclaredDefaults(holder.entries[1]);
    }
}

TEST(ArrayDefault, DefaultedKeysAreWritten) {
    const auto obj = kOnlyRequired.As<ns::ArrayDefaults>();

    for (const auto& json : {TestDomSerializer(obj), TestToJsonString(obj), TestWriteToStream(obj)}) {
        EXPECT_FALSE(json.HasMember("optional_items"));
        EXPECT_FALSE(json.HasMember("null_default"));
        EXPECT_EQ(json["empty_default"], formats::json::MakeArray());
        EXPECT_EQ(json["string_default"], formats::json::MakeArray("sports", "quoted \"news\"\n"));
        EXPECT_EQ(json["custom_empty_default"], formats::json::MakeArray());
        EXPECT_EQ(json["indirect_default"], formats::json::MakeArray("tagged"));
        EXPECT_EQ(json.As<ns::ArrayDefaults>(), obj);
    }
}

USERVER_NAMESPACE_END
