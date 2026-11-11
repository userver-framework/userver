#include <userver/formats/json/masking.hpp>

#include <limits>
#include <string>

#include <fmt/format.h>

#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
namespace {

constexpr auto kUnlimited = std::numeric_limits<std::size_t>::max();

std::string Mask(
    std::string_view input,
    const std::vector<MaskingPath>& paths,
    MaskingListMode mode = MaskingListMode::kBlacklist
) {
    auto result = MaskJson(input, MaskingRules{paths, mode}, kUnlimited);
    EXPECT_FALSE(result.limit_reached);
    return result.text;
}

TEST(JsonMasking, ValueTypesAndSubtrees) {
    for (const auto* scalar : {"null", "true", "false", "1", "-2.3e5", R"("secret")"}) {
        EXPECT_EQ(Mask(scalar, {{}}), R"("...")");
    }
    EXPECT_EQ(
        Mask(R"({"token":{"a":1},"items":[1,2],"ok":true})", {{"token"}, {"items"}}),
        R"({"token":"{...}","items":"[...]","ok":true})"
    );
    EXPECT_EQ(Mask("{}", {{}}), R"("{...}")");
    EXPECT_EQ(Mask("[]", {{}}), R"("[...]")");
}

TEST(JsonMasking, ConfiguredReplacementIsEscapedAndBounded) {
    const MaskingRules rules{{{"scalar"}, {"object"}, {"array"}}, MaskingListMode::kBlacklist, "¢\""};
    constexpr std::string_view kInput = R"({"scalar":42,"object":{"a":1},"array":[1],"public":"..."})";
    const auto result = MaskJson(kInput, rules, kUnlimited);
    EXPECT_FALSE(result.limit_reached);
    EXPECT_EQ(result.text, R"({"scalar":"¢\"","object":"¢\"","array":"¢\"","public":"..."})");

    const auto truncated = MaskJson(kInput, rules, 12);
    EXPECT_TRUE(truncated.limit_reached);
    EXPECT_EQ(truncated.text, R"({"scalar":")");
}

TEST(JsonMasking, PathsThroughArrays) {
    EXPECT_EQ(
        Mask(R"([[{"user":{"token":"a","ok":1}}],{"user":{"token":"b"}}])", {{"user", "token"}}),
        R"([[{"user":{"token":"...","ok":1}}],{"user":{"token":"..."}}])"
    );
    EXPECT_EQ(Mask(R"({"items":[1,[2],{"token":3}]})", {{"items", "token"}}), R"({"items":[1,[2],{"token":"..."}]})");
}

TEST(JsonMasking, RootAndOverlappingPaths) {
    const auto* input = R"({"user":{"token":1},"ok":true})";
    EXPECT_EQ(Mask(input, {{"user"}, {"user", "token"}}), R"({"user":"{...}","ok":true})");
    EXPECT_EQ(Mask(input, {{"user", "token"}, {"user"}}), R"({"user":"{...}","ok":true})");
    EXPECT_EQ(Mask(input, {{"user"}, {}}), R"("{...}")");
}

TEST(JsonMasking, WildcardAndExactPrecedence) {
    const auto* input = R"({"a":{"token":1,"id":2},"b":{"token":3,"id":4}})";
    EXPECT_EQ(Mask(input, {{"*", "token"}}), R"({"a":{"token":"...","id":2},"b":{"token":"...","id":4}})");
    EXPECT_EQ(Mask(input, {{"a", "id"}, {"*", "token"}}), R"({"a":{"token":1,"id":"..."},"b":{"token":"...","id":4}})");
    EXPECT_EQ(
        Mask(input, {{"a", "missing", "token"}, {"*", "token"}}),
        R"({"a":{"token":1,"id":2},"b":{"token":"...","id":4}})"
    );
}

TEST(JsonMasking, StrictWhitelist) {
    const auto mode = MaskingListMode::kWhitelist;
    EXPECT_EQ(
        Mask(R"({"user":{"id":1,"token":2},"other":{},"array":[]})", {{"user", "id"}}, mode),
        R"({"user":{"id":1,"token":"..."},"other":"{...}","array":"[...]"})"
    );
    EXPECT_EQ(Mask(R"({"user":42})", {{"user", "id"}}, mode), R"({"user":"..."})");
    EXPECT_EQ(
        Mask(R"({"user":[1,{"id":2,"token":3}]})", {{"user", "id"}}, mode),
        R"({"user":["...",{"id":2,"token":"..."}]})"
    );
    EXPECT_EQ(Mask("42", {{"user"}}, mode), R"("...")");
    EXPECT_EQ(Mask(R"({"user":{"id":1}})", {{"user"}}, mode), R"({"user":{"id":1}})");
    EXPECT_EQ(Mask(R"({"user":1})", {{}}, mode), R"({"user":1})");
    EXPECT_EQ(Mask("{}", {}, mode), R"("{...}")");
    EXPECT_EQ(Mask("[]", {}, mode), R"("[...]")");
    EXPECT_EQ(Mask("null", {}, mode), R"("...")");
}

TEST(JsonMasking, EscapesDuplicateKeysAndNumbers) {
    EXPECT_EQ(
        Mask(
            R"({"user.token":1,"user":{"token":2},"a\u0000b":3,"token":4,"token":5})",
            {{"user.token"}, {std::string{"a\0b", 3}}, {"token"}}
        ),
        R"({"user.token":"...","user":{"token":2},"a\u0000b":"...","token":"...","token":"..."})"
    );
    const auto* numbers = "[18446744073709551616,-0,1.2300,1e+100]";
    EXPECT_EQ(Mask(numbers, {}), numbers);
    EXPECT_EQ(Mask(" { \"ok\" : true } \n", {}), R"({"ok":true})");
}

TEST(JsonMasking, LimitsAreOutputPrefixes) {
    struct TestCase final {
        const char* name;
        std::string input;
    };
    const TestCase cases[] = {
        {"unicode_values", R"({"token":"secret","ok":"\u00a2\u20ac\ud83d\ude42","items":[1,2,3]})"},
        {"long_string", fmt::format(R"({{"token":"secret","visible":"{}"}})", std::string(256, 's'))},
        {"unicode_key",
         fmt::format(R"({{"{}\u00a2\u20ac\ud83d\ude42{}":true}})", std::string(32, 'k'), std::string(32, 'k'))},
        {"escaped_string", R"({"token":"secret","visible":"\u0000\u0000\n\t\\\"\b\f\r"})"},
        {"long_number", fmt::format(R"({{"visible":0.{}}})", std::string(256, '1'))},
    };
    const MaskingRules rules{{{"token"}}};
    for (const auto& test_case : cases) {
        SCOPED_TRACE(test_case.name);
        const auto full = MaskJson(test_case.input, rules, kUnlimited).text;
        for (std::size_t limit = 0; limit <= full.size() + 1; ++limit) {
            const auto result = MaskJson(test_case.input, rules, limit);
            auto prefix = full.substr(0, limit);
            utils::text::utf8::TrimTruncatedEnding(prefix);
            EXPECT_EQ(result.text, prefix) << limit;
            EXPECT_EQ(result.limit_reached, limit <= full.size()) << limit;
        }
    }
}

TEST(JsonMasking, StopsBeforeInvalidTail) {
    const MaskingRules rules{{{"token"}}};
    const auto* input = R"({"token":"secret","ok":123, invalid SECRET})";
    EXPECT_EQ(MaskJson(input, rules, 14).text, R"({"token":"...")");
    EXPECT_THROW(MaskJson(input, rules, kUnlimited), ParseException);
    EXPECT_TRUE(MaskJson("invalid", rules, 0).limit_reached);
}

TEST(JsonMasking, InvalidInputHasNoPayloadInError) {
    for (const auto& input :
         {std::string{},
          std::string{"{\"SECRET\":}"},
          std::string{"[1,]"},
          std::string{"{} {}"},
          std::string{"{}\0SECRET", 10},
          std::string{"\"\xff\""}})
    {
        try {
            MaskJson(input, MaskingRules{}, kUnlimited);
            FAIL() << "Invalid JSON accepted";
        } catch (const ParseException& ex) {
            EXPECT_EQ(std::string_view{ex.what()}.find("SECRET"), std::string_view::npos);
        }
    }
}

TEST(JsonMasking, DepthLimitIncludesMaskedSubtrees) {
    const std::string input = std::string(129, '[') + "0" + std::string(129, ']');
    EXPECT_THROW(MaskJson(input, MaskingRules{}, kUnlimited), ParseException);
    EXPECT_THROW(MaskJson(input, MaskingRules{{{}}}, kUnlimited), ParseException);
}

}  // namespace
}  // namespace formats::json

USERVER_NAMESPACE_END
