#include <userver/utils/regex.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Regex, Ctors) {
    const utils::regex r1;
    utils::regex r2("regex*test");
    const utils::regex r3(std::move(r2));
    utils::regex r4(r3);
    utils::regex r5;
    r5 = std::move(r4);
    utils::regex r6;
    r6 = r5;
}

TEST(Regex, InvalidRegex) { UEXPECT_THROW(utils::regex("regex***"), utils::RegexError); }

TEST(Regex, NegativeLookaheadDisallowed) {
    UEXPECT_THROW_MSG(utils::regex{"(?!bad)([a-z]+)(\\d*)"}, utils::RegexError, "invalid perl operator: (?!");
}

TEST(Regex, Match) {
    const utils::regex r("^[a-z][0-9]+");
    EXPECT_FALSE(utils::regex_match({}, r));
    EXPECT_FALSE(utils::regex_match("a", r));
    EXPECT_FALSE(utils::regex_match("123", r));
    EXPECT_TRUE(utils::regex_match("a123", r));
    EXPECT_TRUE(utils::regex_match("a1234", r));
    EXPECT_FALSE(utils::regex_match("a123a", r));
}

TEST(Regex, MatchWithResult) {
    const utils::regex r("^[a-z][0-9]+");

    utils::match_results fail;
    constexpr std::string_view str_empty{};
    EXPECT_FALSE(utils::regex_search(str_empty, fail, r));

    utils::match_results success;
    constexpr std::string_view str{"a1234"};
    EXPECT_TRUE(utils::regex_match(str, success, r));
    ASSERT_EQ(success.size(), 1);
    const std::string_view res = success[0];
    EXPECT_EQ(res, str);
}

TEST(Regex, MatchNewlines) {
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
    const utils::regex r("^[a-z][0-9]+");
    EXPECT_FALSE(utils::regex_search({}, r));
    EXPECT_FALSE(utils::regex_search("a", r));
    EXPECT_FALSE(utils::regex_search("123", r));
    EXPECT_TRUE(utils::regex_search("a123", r));
    EXPECT_TRUE(utils::regex_search("a1234", r));
    EXPECT_TRUE(utils::regex_search("a123a", r));
}

TEST(Regex, EmptyRegex) {
    const utils::regex r{""};
    EXPECT_TRUE(utils::regex_search("", r));
    EXPECT_TRUE(utils::regex_match("", r));
}

TEST(Regex, SearchWithResult) {
    const utils::regex r("^[a-z][0-9]+");
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

TEST(Regex, SearchMatchResultsMethods) {
    const utils::regex r("(\\w{2})(\\d)");
    constexpr std::string_view target = " foo  ab4 bar";
    utils::match_results match;
    ASSERT_TRUE(utils::regex_search(target, match, r));

    ASSERT_EQ(match.size(), 3);
    EXPECT_EQ(match[0], "ab4");
    EXPECT_EQ(match[0].data(), target.data() + 6);
    EXPECT_EQ(match.position(0), 6);
    EXPECT_EQ(match.length(0), 3);
    EXPECT_EQ(match[1], "ab");
    EXPECT_EQ(match[1].data(), target.data() + 6);
    EXPECT_EQ(match.position(1), 6);
    EXPECT_EQ(match.length(1), 2);
    EXPECT_EQ(match[2], "4");
    EXPECT_EQ(match[2].data(), target.data() + 8);
    EXPECT_EQ(match.position(2), 8);
    EXPECT_EQ(match.length(2), 1);
    EXPECT_EQ(match.prefix(), " foo  ");
    EXPECT_EQ(match.prefix().data(), target.data());
    EXPECT_EQ(match.suffix(), " bar");
    EXPECT_EQ(match.suffix().data(), target.data() + 9);
}

namespace {

/// [split text]
// An example of complex regex parsing using 'prefix' and 'suffix' methods.
// Suppose that we want to split a text into words and also check that
// the first letter of each sentence is capitalized.
std::vector<std::string_view> SplitTextIntoWords(const std::string_view text) {
    static const utils::regex word_regex("[a-zA-Z]+");
    static const utils::regex punctuation_regex("[., ]*");
    static const utils::regex capitalized_word_start_regex("^[A-Z]");

    std::vector<std::string_view> words;
    utils::match_results word_match;
    auto remaining = text;

    while (utils::regex_search(remaining, word_match, word_regex)) {
        const auto punctuation = word_match.prefix();
        if (!utils::regex_match(punctuation, punctuation_regex)) {
            throw std::invalid_argument(fmt::format("Invalid characters '{}'", punctuation));
        }

        const auto word = word_match[0];
        const bool should_be_capitalized = words.empty() || punctuation.find('.') != std::string_view::npos;
        if (should_be_capitalized && !utils::regex_search(word, capitalized_word_start_regex)) {
            throw std::invalid_argument(fmt::format("Word '{}' should be capitalized", word));
        }

        words.push_back(word);

        remaining = word_match.suffix();
    }

    if (!utils::regex_match(remaining, punctuation_regex)) {
        throw std::invalid_argument(fmt::format("Invalid characters '{}'", remaining));
    }

    return words;
}

TEST(Regex, SplitTextIntoWords) {
    EXPECT_THAT(
        SplitTextIntoWords("Foo bar. Baz,  qux quux."), testing::ElementsAre("Foo", "bar", "Baz", "qux", "quux")
    );
    UEXPECT_THROW_MSG(SplitTextIntoWords("Foo + bar"), std::invalid_argument, "Invalid characters ' + '");
    UEXPECT_THROW_MSG(SplitTextIntoWords("Foo bar. baz."), std::invalid_argument, "Word 'baz' should be capitalized");
    UEXPECT_THROW_MSG(SplitTextIntoWords("Foo, bar% "), std::invalid_argument, "Invalid characters '% '");
}
/// [split text]

}  // namespace

TEST(RegexDeathTest, SearchEmptyCaptureGroupsGoldenTest) {
    // There could be 2 interpretations of this situation:
    // 1. the 2nd capture group of `r` is not present in `str`;
    // 2. the 2nd capture group of `r` is present in `str`, but is empty.
    // The current implementation of utils::regex chooses interpretation (2), but it's not guaranteed.
    const utils::regex r("<([a-z]+)(\\d*)>");
    constexpr std::string_view str = " <abc> ";
    utils::match_results matches;
    ASSERT_TRUE(utils::regex_search(str, matches, r));

    ASSERT_TRUE(matches.size() == 3);
    EXPECT_EQ(matches[0], "<abc>");
    EXPECT_EQ(matches[0].data(), str.data() + 1);
    EXPECT_EQ(matches[1], "abc");
    EXPECT_EQ(matches[1].data(), str.data() + 2);
    EXPECT_EQ(matches[2], "");
    EXPECT_EQ(matches[2].data(), str.data() + 5);  // implementation detail

    EXPECT_EQ(matches.position(0), 1);
    EXPECT_EQ(matches.length(0), 5);
    EXPECT_EQ(matches.position(1), 2);
    EXPECT_EQ(matches.length(1), 3);
    EXPECT_UINVARIANT_FAILURE_MSG(
        matches.position(2),
        "Trying to access position of capturing group 2, which is empty (missing), target=' <abc> '"
    );
    EXPECT_EQ(matches.length(2), 0);

    EXPECT_EQ(matches.prefix(), " ");
    EXPECT_EQ(matches.prefix().data(), str.data());
    EXPECT_EQ(matches.suffix(), " ");
    EXPECT_EQ(matches.suffix().data(), str.data() + 6);
}

TEST(RegexDeathTest, SearchNonPresentCaptureGroupsGoldenTest) {
    // 2nd capture group cannot be present in `r` in any way (otherwise nested <> would have to be present),
    // so utils::regex must return an invalid std::string_view for the 2nd group.
    // The current implementation returns `nullptr` std::string_view, but the exact value of `.data()`
    // is not guaranteed in this case.
    const utils::regex r("<([a-z]+(?:<(\\d*)>)?)>");
    constexpr std::string_view str = " <abc> ";
    utils::match_results matches;
    EXPECT_TRUE(utils::regex_search(str, matches, r));

    ASSERT_TRUE(matches.size() == 3);
    EXPECT_EQ(matches[0], "<abc>");
    EXPECT_EQ(matches[0].data(), str.data() + 1);
    EXPECT_EQ(matches[1], "abc");
    EXPECT_EQ(matches[1].data(), str.data() + 2);
    EXPECT_EQ(matches[2], "");
    EXPECT_EQ(matches[2].data(), nullptr);  // implementation detail

    EXPECT_EQ(matches.position(0), 1);
    EXPECT_EQ(matches.length(0), 5);
    EXPECT_EQ(matches.position(1), 2);
    EXPECT_EQ(matches.length(1), 3);
    EXPECT_UINVARIANT_FAILURE_MSG(
        matches.position(2),
        "Trying to access position of capturing group 2, which is empty (missing), target=' <abc> '"
    );
    EXPECT_EQ(matches.length(2), 0);

    EXPECT_EQ(matches.prefix(), " ");
    EXPECT_EQ(matches.prefix().data(), str.data());
    EXPECT_EQ(matches.suffix(), " ");
    EXPECT_EQ(matches.suffix().data(), str.data() + 6);
}

TEST(RegexDeathTest, SearchEmptyResult) {
    // Create an empty, but non-null string_view.
    constexpr std::string_view kOriginalString = "foo";
    constexpr auto kEmptySubstring = kOriginalString.substr(1, 0);

    const utils::regex r("(\\d*)");
    utils::match_results matches;
    EXPECT_TRUE(utils::regex_search(kEmptySubstring, matches, r));

    ASSERT_EQ(matches.size(), 2);
    EXPECT_EQ(matches[0], "");
    EXPECT_EQ(matches[0].data(), kOriginalString.data() + 1);  // guaranteed
    EXPECT_EQ(matches[1], "");
    EXPECT_EQ(matches[1].data(), kOriginalString.data() + 1);  // implementation detail
    EXPECT_EQ(matches.position(0), 0);
    EXPECT_EQ(matches.length(0), 0);
    EXPECT_UINVARIANT_FAILURE_MSG(
        matches.position(1), "Trying to access position of capturing group 1, which is empty (missing), target=''"
    );
    EXPECT_EQ(matches.length(0), 0);
    EXPECT_EQ(matches.prefix(), "");
    EXPECT_EQ(matches.prefix().data(), kOriginalString.data() + 1);
    EXPECT_EQ(matches.suffix(), "");
    EXPECT_EQ(matches.suffix().data(), kOriginalString.data() + 1);
}

TEST(Regex, Replace) {
    const utils::regex r("[a-z]{2}");
    const std::string repl{"R"};
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

TEST(Regex, ReplaceRe2) {
    const utils::regex r("[a-z]{2}");
    EXPECT_EQ(utils::regex_replace("ab0ef1", r, utils::Re2Replacement{"{\\0}"}), "{ab}0{ef}1");
    EXPECT_EQ(utils::regex_replace("ab0ef1", r, utils::Re2Replacement{"\\\\"}), "\\0\\1");
    const utils::regex group_regex("([a-z]+)(\\d+)");
    EXPECT_EQ(utils::regex_replace("ab0ef1", group_regex, utils::Re2Replacement{"(\\2-\\1)"}), "(0-ab)(1-ef)");
}

USERVER_NAMESPACE_END
