#include <userver/utils/encoding/tskv_parser.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <userver/utest/parameter_names.hpp>
#include <userver/utils/encoding/tskv_parser_read.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [sample]
struct Record {
    std::vector<std::pair<std::string, std::string>> tags;
};
struct Junk {
    std::string data;
};
struct Incomplete {
    std::string data;
};
using SingleParsedRecord = std::variant<Record, Junk, Incomplete>;
using ParsedRecords = std::vector<SingleParsedRecord>;

enum class RecordMode { kAll, kSelected };

ParsedRecords ParseTskv(std::string_view in, RecordMode mode = RecordMode::kAll) {
    ParsedRecords result;
    utils::encoding::TskvParser parser{in};

    const auto account_incomplete = [&](const char* incomplete_begin) {
        const std::string_view remaining(incomplete_begin, in.data() + in.size() - incomplete_begin);
        if (!remaining.empty()) {
            result.push_back(Incomplete{std::string(remaining)});
        }
    };

    while (true) {
        const char* const junk_begin = parser.GetStreamPosition();
        const char* const record_begin = parser.SkipToRecordBegin();

        if (!record_begin) {
            // Note: production code should stop and wait for more data here.
            account_incomplete(junk_begin);
            break;
        }

        // Note: accounting for junk in-between records is supported, but is
        // typically not important in production.
        if (record_begin != junk_begin) {
            result.push_back(Junk{std::string(junk_begin, record_begin)});
        }

        std::vector<std::pair<std::string, std::string>> tags;

        const auto consume = [&](std::string_view key, std::string_view value) {
            tags.emplace_back(key, value);
            return true;
        };
        const auto status =
            mode == RecordMode::kAll
                ? utils::encoding::TskvReadRecord(parser, consume)
                : utils::encoding::TskvReadRecordSelected(parser, [](std::string_view) { return true; }, consume);

        if (status == utils::encoding::TskvParser::RecordStatus::kIncomplete) {
            // Note: production code should stop and wait for more data here.
            account_incomplete(record_begin);
            break;
        }

        result.push_back(Record{std::move(tags)});
    }

    return result;
}
/// [sample]

struct TestCase {
    std::string test_name;
    std::string tskv;
    ParsedRecords result;
};

using TestData = std::initializer_list<TestCase>;

bool operator==(const Record& lhs, const Record& rhs) { return lhs.tags == rhs.tags; }
[[maybe_unused]] void PrintTo(const Record& value, std::ostream* out) { *out << testing::PrintToString(value.tags); }

bool operator==(const Junk& lhs, const Junk& rhs) { return lhs.data == rhs.data; }
[[maybe_unused]] void PrintTo(const Junk& value, std::ostream* out) { *out << value.data; }

bool operator==(const Incomplete& lhs, const Incomplete& rhs) { return lhs.data == rhs.data; }
[[maybe_unused]] void PrintTo(const Incomplete& value, std::ostream* out) { *out << value.data; }

[[maybe_unused]] void PrintTo(const SingleParsedRecord& value, std::ostream* out) {
    *out
        << utils::Visit(
               value,
               [](const Record&) { return "Record"; },
               [](const Junk&) { return "Junk"; },
               [](const Incomplete&) { return "Incomplete"; }
           )
        << "(";
    std::visit([out](const auto& value) { PrintTo(value, out); }, value);
    *out << ")";
}

[[maybe_unused]] void PrintTo(const TestCase& test, std::ostream* out) { *out << test.test_name; }

}  // namespace

class TskvParseTest : public ::testing::TestWithParam<TestCase> {};

TEST_P(TskvParseTest, Parsing) { EXPECT_EQ(ParseTskv(GetParam().tskv), GetParam().result); }

TEST_P(TskvParseTest, ParsingSelected) {
    EXPECT_EQ(ParseTskv(GetParam().tskv, RecordMode::kSelected), GetParam().result);
}

TEST_P(TskvParseTest, SkipValues) {
    utils::encoding::TskvParser reader{GetParam().tskv};
    utils::encoding::TskvParser skipper{GetParam().tskv};
    std::string key;
    std::string skipped_key;
    std::string value;
    while (const auto* begin = reader.SkipToRecordBegin()) {
        ASSERT_EQ(skipper.SkipToRecordBegin(), begin);
        while (true) {
            const auto key_status = reader.ReadKey(key);
            ASSERT_EQ(skipper.ReadKey(skipped_key), key_status);
            ASSERT_EQ(skipped_key, key);
            if (key_status) {
                break;
            }
            const auto value_status = reader.ReadValue(value);
            EXPECT_EQ(skipper.SkipValue(), value_status);
            EXPECT_EQ(skipper.GetStreamPosition(), reader.GetStreamPosition());
            if (value_status) {
                break;
            }
        }
    }
    EXPECT_EQ(skipper.SkipToRecordBegin(), nullptr);
}

TEST(TskvParser, ValueViewBorrowsInputOrScratch) {
    const std::string input = "value\twith\\tescape\tother\\nvalue\n";
    utils::encoding::TskvParser parser{input};
    std::string storage = "untouched";
    std::string_view value;
    EXPECT_EQ(parser.ReadValueView(value, storage), std::nullopt);
    EXPECT_EQ(value, "value");
    EXPECT_EQ(value.data(), input.data());
    EXPECT_EQ(storage, "untouched");
    const auto borrowed_value = value;
    EXPECT_EQ(parser.ReadValueView(value, storage), std::nullopt);
    EXPECT_EQ(value, "with\tescape");
    EXPECT_EQ(value.data(), storage.data());
    const std::string retained_value{value};
    EXPECT_EQ(parser.ReadValueView(value, storage), utils::encoding::TskvParser::RecordStatus::kReachedEnd);
    EXPECT_EQ(value, "other\nvalue");
    EXPECT_EQ(value.data(), storage.data());
    EXPECT_EQ(borrowed_value, "value");
    EXPECT_EQ(retained_value, "with\tescape");
}

TEST(TskvParser, ReadRecordBorrowsUnescapedValues) {
    const std::string input = "tskv\tkey=plain\tkey=with\\tescape\tkey=last\n";
    utils::encoding::TskvParser parser{input};
    ASSERT_NE(parser.SkipToRecordBegin(), nullptr);
    std::vector<std::pair<std::string, std::string>> tags;
    EXPECT_EQ(
        utils::encoding::TskvReadRecord(
            parser,
            [&](std::string_view key, std::string_view value) {
                if (value == "plain" || value == "last") {
                    EXPECT_EQ(value.data(), input.data() + input.find(value));
                }
                tags.emplace_back(key, value);
                return true;
            }
        ),
        utils::encoding::TskvParser::RecordStatus::kReachedEnd
    );
    EXPECT_EQ(
        tags,
        (std::vector<std::pair<std::string, std::string>>{{"key", "plain"}, {"key", "with\tescape"}, {"key", "last"}})
    );
}

TEST(TskvParser, SelectedValuesAndEarlyStop) {
    /// [sample_selected]
    const std::string
        input = "tskv\tother=ignored\\tvalue\tpayload=first\tpayload=second\ttail=ignored\ntskv\tnext=value\n";
    utils::encoding::TskvParser parser{input};
    ASSERT_NE(parser.SkipToRecordBegin(), nullptr);
    std::vector<std::string> values;
    const auto status = utils::encoding::TskvReadRecordSelected(
        parser,
        [](std::string_view key) { return key == "payload"; },
        [&](std::string_view, std::string_view value) {
            values.emplace_back(value);
            return values.size() < 2;
        }
    );
    /// [sample_selected]
    EXPECT_EQ(status, utils::encoding::TskvParser::RecordStatus::kReachedEnd);
    EXPECT_EQ(values, (std::vector<std::string>{"first", "second"}));
    EXPECT_EQ(parser.GetStreamPosition(), input.data() + input.find("tskv\t", 1));
}

TEST(TskvParser, IncompleteValueView) {
    for (const auto* input : {"", "value", "value\\", "value\\t", "\\n\\"}) {
        utils::encoding::TskvParser parser{input};
        std::string storage;
        std::string_view value = "old";
        EXPECT_EQ(parser.ReadValueView(value, storage), utils::encoding::TskvParser::RecordStatus::kIncomplete);
        EXPECT_TRUE(value.empty());
    }
}

TEST(TskvParser, ValueBoundariesMatchOwningAndSkipping) {
    using RecordStatus = utils::encoding::TskvParser::RecordStatus;
    struct ValueCase {
        std::string input;
        std::string decoded;
        std::size_t consumed;
        std::optional<RecordStatus> status;
    };
    const std::vector<ValueCase> cases{
        {"", "", 0, RecordStatus::kIncomplete},
        {"plain", "", 0, RecordStatus::kIncomplete},
        {"prefix\\", "", 0, RecordStatus::kIncomplete},
        {"prefix\\t", "prefix\t", 8, RecordStatus::kIncomplete},
        {"prefix\\tunfinished", "prefix\t", 8, RecordStatus::kIncomplete},
        {"\\x", "\\", 1, RecordStatus::kIncomplete},
        {"plain\tfollowing", "plain", 6, std::nullopt},
        {"plain\nfollowing", "plain", 6, RecordStatus::kReachedEnd},
        {"\\\tfollowing", "\\", 2, std::nullopt},
        {"\\\nfollowing", "\\", 2, RecordStatus::kReachedEnd},
        {"\\\\\nfollowing", "\\", 3, RecordStatus::kReachedEnd},
        {"\\n\\r\\t\\0\\\\\\=\\q\nfollowing", std::string{"\n\r\t\0\\=\\q", 8}, 15, RecordStatus::kReachedEnd},
        {std::string{"a\0b\tfollowing", 13}, std::string{"a\0b", 3}, 4, std::nullopt},
    };
    for (const auto& test : cases) {
        SCOPED_TRACE(test.input);
        utils::encoding::TskvParser reader{test.input};
        utils::encoding::TskvParser viewer{test.input};
        utils::encoding::TskvParser skipper{test.input};
        std::string owned_value = "old";
        std::string storage;
        std::string_view view = "old";
        EXPECT_EQ(reader.ReadValue(owned_value), test.status);
        EXPECT_EQ(owned_value, test.decoded);
        EXPECT_EQ(viewer.ReadValueView(view, storage), test.status);
        EXPECT_EQ(view, test.status == RecordStatus::kIncomplete ? std::string_view{} : test.decoded);
        EXPECT_EQ(skipper.SkipValue(), test.status);
        const auto* expected_position = test.input.data() + test.consumed;
        EXPECT_EQ(reader.GetStreamPosition(), expected_position);
        EXPECT_EQ(viewer.GetStreamPosition(), expected_position);
        EXPECT_EQ(skipper.GetStreamPosition(), expected_position);
    }
}

TEST(TskvParser, SelectionUsesDecodedKeysAndPreservesValueCopies) {
    const std::string
        input = "tskv\tignored=large\\tvalue\tselected\\=key=plain\tselected\\=key=with\\tescape\tselected\\=key\n";
    utils::encoding::TskvParser parser{input};
    ASSERT_NE(parser.SkipToRecordBegin(), nullptr);
    std::vector<std::string> keys;
    std::vector<std::string> values;
    EXPECT_EQ(
        utils::encoding::TskvReadRecordSelected(
            parser,
            [&](std::string_view key) {
                keys.emplace_back(key);
                return key == "selected=key";
            },
            [&](std::string_view key, std::string_view value) {
                EXPECT_EQ(key, "selected=key");
                if (values.empty()) {
                    EXPECT_EQ(value.data(), input.data() + input.find("plain"));
                }
                values.emplace_back(value);
                return true;
            }
        ),
        utils::encoding::TskvParser::RecordStatus::kReachedEnd
    );
    EXPECT_EQ(keys, (std::vector<std::string>{"ignored", "selected=key", "selected=key", "selected=key"}));
    EXPECT_EQ(values, (std::vector<std::string>{"plain", "with\tescape", ""}));
}

TEST(TskvParser, UnselectedValuesDoNotReachConsumer) {
    const std::string input = "tskv\tkey=escaped\\nvalue\tkey_only\tlast=\\q\ntskv\tnext=value\n";
    utils::encoding::TskvParser parser{input};
    ASSERT_NE(parser.SkipToRecordBegin(), nullptr);
    std::vector<std::string> keys;
    EXPECT_EQ(
        utils::encoding::TskvReadRecordSelected(
            parser,
            [&](std::string_view key) {
                keys.emplace_back(key);
                return false;
            },
            [](std::string_view, std::string_view) {
                ADD_FAILURE() << "Unselected values must be skipped";
                return true;
            }
        ),
        utils::encoding::TskvParser::RecordStatus::kReachedEnd
    );
    EXPECT_EQ(keys, (std::vector<std::string>{"key", "key_only", "last"}));
    EXPECT_EQ(parser.GetStreamPosition(), input.data() + input.find("tskv\t", 1));
}

TEST(TskvParser, IncompleteValuesDoNotReachConsumer) {
    for (const auto* input : {"key=", "key=plain", "key=value\\", "key=value\\ntruncated"}) {
        for (const bool selected : {false, true}) {
            SCOPED_TRACE(input);
            SCOPED_TRACE(selected);
            utils::encoding::TskvParser parser{input};
            EXPECT_EQ(
                utils::encoding::TskvReadRecordSelected(
                    parser,
                    [selected](std::string_view) { return selected; },
                    [](std::string_view, std::string_view) {
                        ADD_FAILURE() << "Incomplete values must not reach the consumer";
                        return true;
                    }
                ),
                utils::encoding::TskvParser::RecordStatus::kIncomplete
            );
        }
    }
}

TEST(TskvParser, EarlyStop) {
    using RecordStatus = utils::encoding::TskvParser::RecordStatus;
    using Tags = std::vector<std::pair<std::string, std::string>>;
    struct StopCase {
        std::string input;
        Tags tags;
        RecordStatus status;
        std::vector<std::string_view> remaining;
    };
    const std::vector<StopCase> cases{
        {"first=value\tmiddle=escaped\\nvalue\tlast=end\ntskv\tnext=value\n",
         {{"first", "value"}, {"middle", "escaped\nvalue"}, {"last", "end"}},
         RecordStatus::kReachedEnd,
         {"tskv\tnext=value\n", "tskv\tnext=value\n", "tskv\tnext=value\n"}},
        {"first=value\tmiddle=escaped\\\tvalue\tlast=end\ntskv\tnext=value\n",
         {{"first", "value"}, {"middle", "escaped\\"}, {"value", ""}},
         RecordStatus::kReachedEnd,
         {"tskv\tnext=value\n", "tskv\tnext=value\n", "tskv\tnext=value\n"}},
        {"first=value\tmiddle=escaped\\nvalue\tlast=truncated",
         {{"first", "value"}, {"middle", "escaped\nvalue"}},
         RecordStatus::kIncomplete,
         {"middle=escaped\\nvalue\tlast=truncated", "last=truncated", "truncated"}},
    };
    for (const auto& test : cases) {
        for (std::size_t stop_after = 1; stop_after <= 3; ++stop_after) {
            for (const auto mode : {RecordMode::kAll, RecordMode::kSelected}) {
                SCOPED_TRACE(test.input);
                SCOPED_TRACE(stop_after);
                SCOPED_TRACE(static_cast<int>(mode));
                utils::encoding::TskvParser parser{test.input};
                Tags tags;
                const auto consume = [&](std::string_view key, std::string_view value) {
                    tags.emplace_back(key, value);
                    return tags.size() < stop_after;
                };
                const auto status =
                    mode == RecordMode::kAll
                        ? utils::encoding::TskvReadRecord(parser, consume)
                        : utils::encoding::TskvReadRecordSelected(
                              parser,
                              [](std::string_view) { return true; },
                              consume
                          );
                EXPECT_EQ(status, test.status);
                EXPECT_EQ(tags, (Tags{test.tags.begin(), test.tags.begin() + std::min(stop_after, test.tags.size())}));
                EXPECT_EQ(
                    parser.GetStreamPosition(),
                    test.input.data() + test.input.size() - test.remaining[stop_after - 1].size()
                );
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(  //
    SingleLine,
    TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"1_field", "tskv\thello=word\n", {Record{{{"hello", "word"}}}}},

        {"2_fields", "tskv\thello=word\tfoo=bar\n", {Record{{{"hello", "word"}, {"foo", "bar"}}}}},

        {"2_field_and_eq_sign",
         "tskv\thello\\=there=word\tfoo\\=there=bar\n",
         {Record{{{"hello=there", "word"}, {"foo=there", "bar"}}}}},

        {"2_fields_and_many_eq_signs",
         "tskv\t\\=hello=word\t\\=foo\\=\\=there\\==bar\n",
         {Record{{{"=hello", "word"}, {"=foo==there=", "bar"}}}}},

        {"2_fields_and_many_tabs",
         "tskv\thello=\\t\\tw\\t\\to\\tr\\td\\t\\t\tfoo\\=there=\\tbar\n",
         {Record{{{"hello", "\t\tw\t\to\tr\td\t\t"}, {"foo=there", "\tbar"}}}}},

        {"2_fields_and_many_newlines",
         "tskv\thello=\\n\\nw\\n\\no\\nr\\nd\\n\\n\tfoo\\=there=\\nbar\n",
         {Record{{{"hello", "\n\nw\n\no\nr\nd\n\n"}, {"foo=there", "\nbar"}}}}},

        {"3_fields_with_empty_key_values_fixed",
         "tskv\t=word\tfoo=\t=\n",
         {Record{{{"", "word"}, {"foo", ""}, {"", ""}}}}},

        {"3_fields_with_empty_key_values_and_eq_signs",
         "tskv\t\\===\tfoo==\t==\n",
         {Record{{{"=", "="}, {"foo", "="}, {"", "="}}}}},

        {"key_no_equals", "tskv\tkey1\tkey2\n", {Record{{{"key1", ""}, {"key2", ""}}}}},

        {"empty_record", "tskv\t\n", {Record{{}}}},

        {"just_empty", "", {}},
    }),
    utest::PrintTestName{}
);

INSTANTIATE_TEST_SUITE_P(  //
    IncompleteSingleLine,
    TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"no_newline", "tskv\th=w", {Incomplete{"tskv\th=w"}}},
        {"just_t", "t", {Incomplete{"t"}}},
        {"t_newline", "t\n", {Incomplete{"t\n"}}},
        {"tskv_newline", "tskv\n", {Incomplete{"tskv\n"}}},
        {"just_newline", "\n", {Incomplete{"\n"}}},
        {"newline_tskv", "\ntskv", {Incomplete{"\ntskv"}}},
        {"just_tskv_header", "\ntskv\t", {Junk{"\n"}, Incomplete{"tskv\t"}}},
        {"no_newline_2", "\ntskv\tk", {Junk{"\n"}, Incomplete{"tskv\tk"}}},
        {"no_newline_3", "\ntskv\tk=", {Junk{"\n"}, Incomplete{"tskv\tk="}}},
        {"no_newline_4", "\ntskv\tk=v", {Junk{"\n"}, Incomplete{"tskv\tk=v"}}},
    }),
    utest::PrintTestName{}
);

INSTANTIATE_TEST_SUITE_P(  //
    BrokenStartLine,
    TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"start_newline", "\ntskv\tk=v\n", {Junk{"\n"}, Record{{{"k", "v"}}}}},
        {"start_q_newline", "q\ntskv\tk=v\n", {Junk{"q\n"}, Record{{{"k", "v"}}}}},
        {"start_q_newline_newline", "q\n\ntskv\tk=v\n", {Junk{"q\n\n"}, Record{{{"k", "v"}}}}},
        {"start_q123_newline", "q123\ntskv\tk=v\n", {Junk{"q123\n"}, Record{{{"k", "v"}}}}},
        {"start_multiple_junk_newlines", "1\n2\n3\ntskv\tk=v\n", {Junk{"1\n2\n3\n"}, Record{{{"k", "v"}}}}},
    }),
    utest::PrintTestName{}
);

INSTANTIATE_TEST_SUITE_P(  //
    EscapeSingleLine,
    TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"quotes", "tskv\t\"k\"=\"v\"\t\"=\"\n", {Record{{{"\"k\"", "\"v\""}, {"\"", "\""}}}}},

        {"quotes_backslashes",
         "tskv\t\\\"k\\\"=\\\"v\\\"\t\\\"=\\\"\n",
         {Record{{{"\\\"k\\\"", "\\\"v\\\""}, {"\\\"", "\\\""}}}}},

        // This test contains multiple invalid (hanging) backslashes. We can
        // potentially give out arbitrary result for those. Still the parser
        // must not skip \t and \n control characters because of them.
        {"backslashes", "tskv\t\\k=\\v\\\t\\\\=\\\n", {Record{{{"\\k", "\\v\\"}, {"\\", "\\"}}}}},

        {"backslashes_fixed", "tskv\t\\k=\\v\\\\\t\\\\=\\\\\n", {Record{{{"\\k", "\\v\\"}, {"\\", "\\"}}}}},

        {"backslashes_and_empty_fixed", "tskv\t\\\\k=\\\\v\\\\\t=\\\\\n", {Record{{{"\\k", "\\v\\"}, {"", "\\"}}}}},

        {"carriage_returns", "tskv\t\rk\r=\rv\r\t\r=\r\n", {Record{{{"\rk\r", "\rv\r"}, {"\r", "\r"}}}}},

        {"code_1_char",
         "tskv\t\1k\1=\1v\1\t\1=\1\n",
         {Record{{{"\u0001k\u0001", "\u0001v\u0001"}, {"\u0001", "\u0001"}}}}},

        {"newline_escape_fixed", "tskv\t\\rk\\r=\\nv\\n\t\\n=\\b\n", {Record{{{"\rk\r", "\nv\n"}, {"\n", "\\b"}}}}},

        {"control_char_1f",
         "tskv\t\x1fk\x1f=\x1fv\x1f\t\x1f=\x1f\n",
         {Record{{{"\u001fk\u001f", "\u001fv\u001f"}, {"\u001f", "\u001f"}}}}},
    }),
    utest::PrintTestName{}
);

INSTANTIATE_TEST_SUITE_P(
    SingleLineEmpties,
    TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"key_missing", "tskv\t=world\n", {Record{{{"", "world"}}}}},
        {"value_missing", "tskv\thello=\n", {Record{{{"hello", ""}}}}},
        {"key_value_missing_equals", "tskv\t=\n", {Record{{{"", ""}}}}},
        {"middle_double_tab", "tskv\t\tfoo=bar\n", {Record{{{"", ""}, {"foo", "bar"}}}}},
        {"trailing_tab_newline", "tskv\tfoo=bar\t\n", {Record{{{"foo", "bar"}}}}},
        {"trailing_tab_newline_single", "tskv\t\n", {Record{{}}}},
        {"multiple_values_missing",
         "tskv\tt=\tmodule=\tlevel=\t\n",
         {Record{{{"t", ""}, {"module", ""}, {"level", ""}}}}},
    }),
    utest::PrintTestName{}
);

INSTANTIATE_TEST_SUITE_P(
    MultiLine,
    TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"multiple_logs",
         "tskv\tlevel=ERROR\t_type=testing_error_log\n"
         "tskv\tlevel=INFO\t_type=testing_non_error_log\n"
         "tskv\tlevel=INFO\ttype=testing_another_non_error_log\n"
         "tskv\tlevel=ERROR\t\n",
         {
             Record{{{"level", "ERROR"}, {"_type", "testing_error_log"}}},
             Record{{{"level", "INFO"}, {"_type", "testing_non_error_log"}}},
             Record{{{"level", "INFO"}, {"type", "testing_another_non_error_log"}}},
             Record{{{"level", "ERROR"}}},
         }},
    }),
    utest::PrintTestName{}
);

USERVER_NAMESPACE_END
