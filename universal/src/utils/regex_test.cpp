#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

auto AllowBoostRegexScope(bool allow) {
    const auto old = utils::IsImplicitBoostRegexFallbackAllowed();
    utils::SetImplicitBoostRegexFallbackAllowed(allow);
    return utils::FastScopeGuard([old]() noexcept { utils::SetImplicitBoostRegexFallbackAllowed(old); });
}

#ifndef USERVER_NO_RE2_SUPPORT
constexpr bool kHasRe2Support = true;
#else
constexpr bool kHasRe2Support = false;
#endif

}  // namespace

TEST(Regex, Ctors) {
    utils::regex r1;
    utils::regex r2("regex*test");
    utils::regex r3(std::move(r2));
    utils::regex r4(r3);
    utils::regex r5;
    r5 = std::move(r4);
    utils::regex r6;
    r6 = r5;
}

TEST(Regex, InvalidRegex) { UEXPECT_THROW(utils::regex("regex***"), utils::RegexError); }

TEST(Regex, NegativeLookaheadDisallowed) {
    if constexpr (!kHasRe2Support) {
        GTEST_SKIP() << "No re2 support";
    }

    const auto allowed = AllowBoostRegexScope(false);
    UEXPECT_THROW_MSG(
        utils::regex{"(?!bad)([a-z]+)(\\d*)"},
        utils::RegexError,
        "Note: boost::regex usage in utils::regex is disallowed for the current service"
    );
}

TEST(Regex, Match) {
    utils::regex r("^[a-z][0-9]+");
    EXPECT_FALSE(utils::regex_match({}, r));
    EXPECT_FALSE(utils::regex_match("a", r));
    EXPECT_FALSE(utils::regex_match("123", r));
    EXPECT_TRUE(utils::regex_match("a123", r));
    EXPECT_TRUE(utils::regex_match("a1234", r));
    EXPECT_FALSE(utils::regex_match("a123a", r));
}

TEST(Regex, MatchWithResult) {
    utils::regex r("^[a-z][0-9]+");
    utils::match_results fail;
    const std::string str_empty{};
    EXPECT_FALSE(utils::regex_search(str_empty, fail, r));
    ASSERT_EQ(fail.size(), 1);
    const std::string_view empty = fail[0];
    EXPECT_EQ(empty, str_empty);
    utils::match_results success;
    const std::string str{"a1234"};
    EXPECT_TRUE(utils::regex_match(str, success, r));
    ASSERT_EQ(success.size(), 1);
    const std::string_view res = success[0];
    EXPECT_EQ(res, str);
}

TEST(Regex, MatchNegativeLookahead) {
    const auto allowed = AllowBoostRegexScope(true);
    const utils::regex r("(?!bad)([a-z]+)(\\d*)");
    EXPECT_TRUE(utils::regex_match("good42", r));
    EXPECT_FALSE(utils::regex_match("bad42", r));

    utils::match_results match;
    ASSERT_TRUE(utils::regex_match("good42", match, r));
    ASSERT_EQ(match.size(), 3);
    EXPECT_EQ(match[0], "good42");
    EXPECT_EQ(match[1], "good");
    EXPECT_EQ(match[2], "42");

    EXPECT_FALSE(utils::regex_match("bad", match, r));
}

TEST(Regex, MatchNewlines) {
    if constexpr (!kHasRe2Support) {
        GTEST_SKIP() << "No re2 support";
    }

    // $ matches the end of the whole string as a safe default.
    const utils::regex r1(R"(^(1\n2\n3)(\n)?$)");
    EXPECT_TRUE(utils::regex_search("1\n2\n3\n", r1));
    EXPECT_FALSE(utils::regex_search("1\n2\n3\n\n", r1));
    EXPECT_FALSE(utils::regex_search("1\n2\n3\n'); DROP TABLE students;--", r1));

    // (?m) allows $ to match the end of a line.
    const utils::regex r2(R"((?m)^(1\n2\n3)(\n)?$)");
    EXPECT_TRUE(utils::regex_search("1\n2\n3\n", r2));
    EXPECT_TRUE(utils::regex_search("1\n2\n3\n\n", r2));
    EXPECT_TRUE(utils::regex_search("1\n2\n3\n'); DROP TABLE students;--", r2));
}

TEST(Regex, Search) {
    utils::regex r("^[a-z][0-9]+");
    EXPECT_FALSE(utils::regex_search({}, r));
    EXPECT_FALSE(utils::regex_search("a", r));
    EXPECT_FALSE(utils::regex_search("123", r));
    EXPECT_TRUE(utils::regex_search("a123", r));
    EXPECT_TRUE(utils::regex_search("a1234", r));
    EXPECT_TRUE(utils::regex_search("a123a", r));
}

TEST(Regex, EmptyRegex) {
    utils::regex r{""};
    EXPECT_TRUE(utils::regex_search("", r));
    EXPECT_TRUE(utils::regex_match("", r));
}

TEST(Regex, SearchWithResult) {
    utils::regex r("^[a-z][0-9]+");
    utils::match_results fail;
    const std::string str_empty{};
    EXPECT_FALSE(utils::regex_search(str_empty, fail, r));
    {
        // Note: testing an implementation detail here
        ASSERT_EQ(fail.size(), 1);
        const std::string_view empty = fail[0];
        EXPECT_EQ(empty, str_empty);
    }
    utils::match_results success;
    const std::string str{"a1234"};
    EXPECT_TRUE(utils::regex_search(str, success, r));
    ASSERT_EQ(success.size(), 1);
    const std::string_view res = success[0];
    EXPECT_EQ(res, str);
}

TEST(Regex, SearchNegativeLookahead) {
    const auto allowed = AllowBoostRegexScope(true);
    const utils::regex r("(?!bad)([a-z]{3,})(\\d*)");
    EXPECT_TRUE(utils::regex_search(" 42 good42 ", r));
    EXPECT_FALSE(utils::regex_search("   bad42 ", r));

    utils::match_results match;
    ASSERT_TRUE(utils::regex_search(" @  good42 ", match, r));
    ASSERT_EQ(match.size(), 3);
    EXPECT_EQ(match[0], "good42");
    EXPECT_EQ(match[1], "good");
    EXPECT_EQ(match[2], "42");

    EXPECT_FALSE(utils::regex_search("  bad42 ", match, r));
}

TEST(Regex, SearchEmptyCaptureGroupsGoldenTest) {
    // There could be 2 interpretations of this situation:
    // 1. the 2nd capture group of `r` is not present in `str`;
    // 2. the 2nd capture group of `r` is present in `str`, but is empty.
    // The current implementation of utils::regex chooses interpretation (2), but it's not guaranteed.
    const utils::regex r("<([a-z]+)(\\d*)>");
    constexpr std::string_view str = " <abc> ";
    utils::match_results matches;
    EXPECT_TRUE(utils::regex_search(str, matches, r));
    EXPECT_EQ(matches[0], "<abc>");
    EXPECT_EQ(matches[0].data(), str.data() + 1);
    EXPECT_EQ(matches[1], "abc");
    EXPECT_EQ(matches[1].data(), str.data() + 2);
    EXPECT_EQ(matches[2], "");
    EXPECT_EQ(matches[2].data(), str.data() + 5);
}

TEST(Regex, SearchNonPresentCaptureGroupsGoldenTest) {
    if constexpr (!kHasRe2Support) {
        GTEST_SKIP() << "No re2 support";
    }

    // 2nd capture group cannot be present in `r` in any way (otherwise nested <> would have to be present),
    // so utils::regex must return an invalid std::string_view for the 2nd group.
    // The current implementation returns `nullptr` std::string_view, but the exact value of `.data()`
    // is not guaranteed in this case.
    const utils::regex r("<([a-z]+(?:<(\\d*)>)?)>");
    constexpr std::string_view str = " <abc> ";
    utils::match_results matches;
    EXPECT_TRUE(utils::regex_search(str, matches, r));
    EXPECT_EQ(matches[0], "<abc>");
    EXPECT_EQ(matches[0].data(), str.data() + 1);
    EXPECT_EQ(matches[1], "abc");
    EXPECT_EQ(matches[1].data(), str.data() + 2);
    EXPECT_EQ(matches[2], "");
    EXPECT_EQ(matches[2].data(), nullptr);
}

TEST(Regex, Replace) {
    utils::regex r("[a-z]{2}");
    std::string repl{"R"};
    EXPECT_EQ(utils::regex_replace({}, r, repl), "");
    EXPECT_EQ(utils::regex_replace({"a0AB1c2"}, r, repl), "a0AB1c2");
    EXPECT_EQ(utils::regex_replace("ab0ef1", r, repl), "R0R1");
    EXPECT_EQ(utils::regex_replace("abcd", r, repl), "RR");

    // Intentionally does not support substitutions.
    EXPECT_EQ(utils::regex_replace("ab0ef1", r, "\\0"), "\\00\\01");
}

TEST(Regex, ReplaceEmpty) {
    const utils::regex r("\\d*");
    EXPECT_EQ(utils::regex_replace("abcd", r, "*"), "*a*b*c*d*");
    // Here, "123" and the empty string after "123" are both matched, but not the "" before "123".
    // This nuance may vary between different implementations.
    // The main point is to ensure that the call never goes into an infinite loop or crashes.
    EXPECT_EQ(utils::regex_replace("ab123cd", r, "*"), "*a*b**c*d*");
}

#ifndef USERVER_NO_RE2_SUPPORT

TEST(Regex, ReplaceRe2) {
    const utils::regex r("[a-z]{2}");
    EXPECT_EQ(utils::regex_replace("ab0ef1", r, utils::Re2Replacement{"{\\0}"}), "{ab}0{ef}1");
    EXPECT_EQ(utils::regex_replace("ab0ef1", r, utils::Re2Replacement{"\\\\"}), "\\0\\1");
    const utils::regex group_regex("([a-z]+)(\\d+)");
    EXPECT_EQ(utils::regex_replace("ab0ef1", group_regex, utils::Re2Replacement{"(\\2-\\1)"}), "(0-ab)(1-ef)");
}

#endif

USERVER_NAMESPACE_END
