#include <userver/utils/encoding/tskv_parser.hpp>

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

ParsedRecords ParseTskv(std::string_view in) {
  ParsedRecords result;
  utils::encoding::TskvParser parser{in};

  const auto account_incomplete = [&](const char* incomplete_begin) {
    const std::string_view remaining(incomplete_begin,
                                     in.data() + in.size() - incomplete_begin);
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

    const auto status = utils::encoding::TskvReadRecord(
        parser, [&](const std::string& key, const std::string& value) {
          tags.emplace_back(key, value);
          return true;
        });

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

bool operator==(const Record& lhs, const Record& rhs) {
  return lhs.tags == rhs.tags;
}
[[maybe_unused]] void PrintTo(const Record& value, std::ostream* out) {
  *out << testing::PrintToString(value.tags);
}

bool operator==(const Junk& lhs, const Junk& rhs) {
  return lhs.data == rhs.data;
}
[[maybe_unused]] void PrintTo(const Junk& value, std::ostream* out) {
  *out << value.data;
}

bool operator==(const Incomplete& lhs, const Incomplete& rhs) {
  return lhs.data == rhs.data;
}
[[maybe_unused]] void PrintTo(const Incomplete& value, std::ostream* out) {
  *out << value.data;
}

[[maybe_unused]] void PrintTo(const SingleParsedRecord& value,
                              std::ostream* out) {
  *out << utils::Visit(
              value, [](const Record&) { return "Record"; },
              [](const Junk&) { return "Junk"; },
              [](const Incomplete&) { return "Incomplete"; })
       << "(";
  std::visit([out](const auto& value) { PrintTo(value, out); }, value);
  *out << ")";
}

[[maybe_unused]] void PrintTo(const TestCase& test, std::ostream* out) {
  *out << test.test_name;
}

}  // namespace

class TskvParseTest : public ::testing::TestWithParam<TestCase> {};

TEST_P(TskvParseTest, Parsing) {
  EXPECT_EQ(ParseTskv(GetParam().tskv), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(  //
    SingleLine, TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"1_field", "tskv\thello=word\n", {Record{{{"hello", "word"}}}}},

        {"2_fields",
         "tskv\thello=word\tfoo=bar\n",
         {Record{{{"hello", "word"}, {"foo", "bar"}}}}},

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

        {"key_no_equals",
         "tskv\tkey1\tkey2\n",
         {Record{{{"key1", ""}, {"key2", ""}}}}},

        {"empty_record", "tskv\t\n", {Record{{}}}},

        {"just_empty", "", {}},
    }),
    utest::PrintTestName{});

INSTANTIATE_TEST_SUITE_P(  //
    IncompleteSingleLine, TskvParseTest,
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
    utest::PrintTestName{});

INSTANTIATE_TEST_SUITE_P(  //
    BrokenStartLine, TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"start_newline", "\ntskv\tk=v\n", {Junk{"\n"}, Record{{{"k", "v"}}}}},
        {"start_q_newline",
         "q\ntskv\tk=v\n",
         {Junk{"q\n"}, Record{{{"k", "v"}}}}},
        {"start_q_newline_newline",
         "q\n\ntskv\tk=v\n",
         {Junk{"q\n\n"}, Record{{{"k", "v"}}}}},
        {"start_q123_newline",
         "q123\ntskv\tk=v\n",
         {Junk{"q123\n"}, Record{{{"k", "v"}}}}},
        {"start_multiple_junk_newlines",
         "1\n2\n3\ntskv\tk=v\n",
         {Junk{"1\n2\n3\n"}, Record{{{"k", "v"}}}}},
    }),
    utest::PrintTestName{});

INSTANTIATE_TEST_SUITE_P(  //
    EscapeSingleLine, TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"quotes",
         "tskv\t\"k\"=\"v\"\t\"=\"\n",
         {Record{{{"\"k\"", "\"v\""}, {"\"", "\""}}}}},

        {"quotes_backslashes",
         "tskv\t\\\"k\\\"=\\\"v\\\"\t\\\"=\\\"\n",
         {Record{{{"\\\"k\\\"", "\\\"v\\\""}, {"\\\"", "\\\""}}}}},

        // This test contains multiple invalid (hanging) backslashes. We can
        // potentially give out arbitrary result for those. Still the parser
        // must not skip \t and \n control characters because of them.
        {"backslashes",
         "tskv\t\\k=\\v\\\t\\\\=\\\n",
         {Record{{{"\\k", "\\v\\"}, {"\\", "\\"}}}}},

        {"backslashes_fixed",
         "tskv\t\\k=\\v\\\\\t\\\\=\\\\\n",
         {Record{{{"\\k", "\\v\\"}, {"\\", "\\"}}}}},

        {"backslashes_and_empty_fixed",
         "tskv\t\\\\k=\\\\v\\\\\t=\\\\\n",
         {Record{{{"\\k", "\\v\\"}, {"", "\\"}}}}},

        {"carriage_returns",
         "tskv\t\rk\r=\rv\r\t\r=\r\n",
         {Record{{{"\rk\r", "\rv\r"}, {"\r", "\r"}}}}},

        {"code_1_char",
         "tskv\t\1k\1=\1v\1\t\1=\1\n",
         {Record{{{"\u0001k\u0001", "\u0001v\u0001"}, {"\u0001", "\u0001"}}}}},

        {"newline_escape_fixed",
         "tskv\t\\rk\\r=\\nv\\n\t\\n=\\b\n",
         {Record{{{"\rk\r", "\nv\n"}, {"\n", "\\b"}}}}},

        {"control_char_1f",
         "tskv\t\x1fk\x1f=\x1fv\x1f\t\x1f=\x1f\n",
         {Record{{{"\u001fk\u001f", "\u001fv\u001f"}, {"\u001f", "\u001f"}}}}},
    }),
    utest::PrintTestName{});

INSTANTIATE_TEST_SUITE_P(
    SingleLineEmpties, TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"key_missing", "tskv\t=world\n", {Record{{{"", "world"}}}}},
        {"value_missing", "tskv\thello=\n", {Record{{{"hello", ""}}}}},
        {"key_value_missing_equals", "tskv\t=\n", {Record{{{"", ""}}}}},
        {"middle_double_tab",
         "tskv\t\tfoo=bar\n",
         {Record{{{"", ""}, {"foo", "bar"}}}}},
        {"trailing_tab_newline",
         "tskv\tfoo=bar\t\n",
         {Record{{{"foo", "bar"}}}}},
        {"trailing_tab_newline_single", "tskv\t\n", {Record{{}}}},
        {"multiple_values_missing",
         "tskv\tt=\tmodule=\tlevel=\t\n",
         {Record{{{"t", ""}, {"module", ""}, {"level", ""}}}}},
    }),
    utest::PrintTestName{});

INSTANTIATE_TEST_SUITE_P(
    MultiLine, TskvParseTest,
    ::testing::ValuesIn(TestData{
        {"multiple_logs",
         "tskv\tlevel=ERROR\t_type=testing_error_log\n"
         "tskv\tlevel=INFO\t_type=testing_non_error_log\n"
         "tskv\tlevel=INFO\ttype=testing_another_non_error_log\n"
         "tskv\tlevel=ERROR\t\n",
         {
             Record{{{"level", "ERROR"}, {"_type", "testing_error_log"}}},
             Record{{{"level", "INFO"}, {"_type", "testing_non_error_log"}}},
             Record{{{"level", "INFO"},
                     {"type", "testing_another_non_error_log"}}},
             Record{{{"level", "ERROR"}}},
         }},
    }),
    utest::PrintTestName{});

USERVER_NAMESPACE_END
