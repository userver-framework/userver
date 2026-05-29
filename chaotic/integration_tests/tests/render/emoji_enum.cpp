#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/formats/parse/variant.hpp>  //  TODO: should work without it

#include <gmock/gmock.h>

#include <string>
#include <vector>

#include <schemas/emoji_enum.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Formatter, EmojiEnum) {
    auto obj1 = formats::json::MakeObject("emoji", "👉").As<ns::EnumEmojiThe>();
    auto obj2 = formats::json::MakeObject("emoji", "⚡").As<ns::EnumEmoji>();
    auto obj3 = formats::json::MakeObject("emoji", "🟤").As<ns::EnumEmoji>();
    auto obj4 = formats::json::MakeObject("emoji", "🦊").As<ns::EnumEmoji>();
    auto obj5 = formats::json::MakeObject("emoji", "🦘").As<ns::EnumEmoji>();
    auto obj6 = formats::json::MakeObject("emoji", "over").As<ns::EnumEmoji>();
    auto obj7 = formats::json::MakeObject("emoji", "lazy🐶").As<ns::EnumEmoji>();
    std::vector<ns::EnumEmoji> objects{obj1, obj2, obj3, obj4, obj5, obj6, obj7};

    std::string phrase;

    for (const auto& obj : objects) {
        phrase += std::visit([](const auto& item) -> std::string { return item.text; }, obj);
    }
    EXPECT_EQ(phrase, "the quick brown fox jumps over lazy dog");
}

USERVER_NAMESPACE_END
