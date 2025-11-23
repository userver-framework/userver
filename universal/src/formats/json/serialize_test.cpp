#include <gtest/gtest.h>

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>

#include <formats/common/serialize_test.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/parser/parser.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct Serialization<formats::json::Value> : public ::testing::Test {
    constexpr static const char* kDoc = R"({"key1":1,"key2":"val"})";

    using ValueBuilder = formats::json::ValueBuilder;
    using Value = formats::json::Value;
    using Type = formats::json::Type;

    using ParseException = formats::json::ParseException;
    using TypeMismatchException = formats::json::TypeMismatchException;
    using OutOfBoundsException = formats::json::OutOfBoundsException;
    using MemberMissingException = formats::json::MemberMissingException;
    using BadStreamException = formats::json::BadStreamException;

    constexpr static auto kFromString = formats::json::FromString;
    constexpr static auto kFromStream = formats::json::FromStream;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Serialization, formats::json::Value);

namespace {

void TestExceptionMessage(std::string_view data, std::string_view expected_msg) {
    using formats::json::FromString;
    using ParseException = formats::json::Value::ParseException;

    try {
        FromString(data);
        FAIL() << "Exception was not thrown on json: " << data;
    } catch (const ParseException& e) {
        EXPECT_TRUE(std::string_view{e.what()}.find(expected_msg) != std::string_view::npos)
            << "JSON " << data << " has incorrect line/column error message: " << e.what();
    }
}

}  // namespace

TEST(FormatsJson, ParseErrorLineColumnValidation) {
    TestExceptionMessage(
        R"~({
}})~",
        "line 2 column 2"
    );

    TestExceptionMessage(
        R"~(}{
})~",
        "line 1 column 1"
    );

    TestExceptionMessage(
        R"~({
"foo":"bar":"buz"
})~",
        "line 2 column 12"
    );
}

TEST(FormatsJson, ParseFromBadFile) {
    using formats::json::blocking::FromFile;
    using ParseException = formats::json::Value::ParseException;

    const char* filename = "@ file that / does not exist >< &";
    try {
        FromFile(filename);
        FAIL() << "Exception was not thrown for non existing file";
    } catch (const ParseException& e) {
        EXPECT_TRUE(std::string_view{e.what()}.find(filename) != std::string_view::npos)
            << "No filename in error message: " << e.what();
    }
}

TEST(FormatsJson, ParseStringHasZeroByte) {
    TestExceptionMessage(
        std::string_view{"{}\0z", 4},
        "JSON parse error at line 1 column 3: The document root must not be followed by other values."
    );
}

TEST(FormatsJson, ParseStreamHasZeroByte) {
    std::istringstream is(std::string{"{}\0z", 4});

    try {
        formats::json::FromStream(is);
        FAIL() << "Exception was not thrown";
    } catch (const formats::json::ParseException& exc) {
        EXPECT_EQ(
            std::string_view{exc.what()},
            std::string_view{"JSON parse error at offset 2: The document root must not be followed by other values."}
        );
    }
}

class FmtFormatterParameterized : public testing::TestWithParam<std::string> {};

TEST_P(FmtFormatterParameterized, FormatsJsonFmt) {
    const std::string str = GetParam();
    const auto value = formats::json::FromString(str);
    EXPECT_EQ(fmt::format("{}", value), str);
    EXPECT_THROW(static_cast<void>(fmt::format(fmt::runtime("{:s}"), value)), fmt::format_error);
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    FmtFormatterParameterized,
    testing::Values(R"({"field":123})", "null", "12345", "123.45", R"(["abc","def"])")
);

TEST(JsonToSortedString, Null) {
    const formats::json::Value example = formats::json::FromString("null");
    ASSERT_EQ(formats::json::ToStableString(example), "null");
}

namespace {

struct NotSortedTestData {
    std::string source;
    std::string result;
};

struct CycleTestData {
    std::string source;
};

class JsonToStringCycle : public ::testing::TestWithParam<CycleTestData> {};

class NonSortedJsonToSortedString : public ::testing::TestWithParam<NotSortedTestData> {};

const auto global_json1 = formats::json::FromString(R"({"b":{"b":{"b":1}},"c":{"c":{"c":1}},"a":1})");
const auto global_json2 = formats::json::FromString(R"({"c":1,"b":{"b":{"b":1,"a":1}},"a":1})");
const auto global_json3 = formats::json::FromString(R"({"b":{"b":{"b":1,"a":1}},"a":1,"c":1})");
thread_local const auto thread_local_json1 = global_json1;
thread_local const auto thread_local_json2 = global_json2;
thread_local const auto thread_local_json3 = global_json3;

}  // namespace

TEST_P(NonSortedJsonToSortedString, NonDepthTreeCopy) {
    const NotSortedTestData pair_data_res = GetParam();
    const auto json = formats::json::FromString(pair_data_res.source);
    EXPECT_EQ(formats::json::ToStableString(json), pair_data_res.result);
}

TEST_P(NonSortedJsonToSortedString, NonDepthTreeMove) {
    const NotSortedTestData pair_data_res = GetParam();
    auto json = formats::json::FromString(pair_data_res.source);
    EXPECT_EQ(formats::json::ToStableString(std::move(json)), pair_data_res.result);
}

TEST_P(JsonToStringCycle, NonDepthTree) {
    const CycleTestData data = GetParam();
    const auto json = formats::json::FromString(data.source);
    const auto json_str = formats::json::ToString(json);
    const auto json_copy = formats::json::FromString(json_str);
    EXPECT_EQ(formats::json::ToStableString(json_copy), formats::json::ToStableString(json));
}

INSTANTIATE_TEST_SUITE_P(
    JsonToSortedString,
    NonSortedJsonToSortedString,
    ::testing::Values(
        NotSortedTestData{"null", "null"},
        NotSortedTestData{"false", "false"},
        NotSortedTestData{"true", "true"},
        NotSortedTestData{"{}", "{}"},
        NotSortedTestData{"[]", "[]"},
        NotSortedTestData{"1", "1"},
        NotSortedTestData{R"({"b":1,"a":1})", R"({"a":1,"b":1})"},
        NotSortedTestData{R"({"b":1,"a":1,"c":1})", R"({"a":1,"b":1,"c":1})"},
        NotSortedTestData{R"({"b":{"b":1,"a":1}, "a":1,"c":1})", R"({"a":1,"b":{"a":1,"b":1},"c":1})"},
        NotSortedTestData{R"({"b":1,"a":1,"c":{"b":1,"a":1}})", R"({"a":1,"b":1,"c":{"a":1,"b":1}})"},
        NotSortedTestData{R"({"b":{"b":{"b":1}},"a":1,"c":1})", R"({"a":1,"b":{"b":{"b":1}},"c":1})"},
        NotSortedTestData{R"({"a":1,"c":1,"b":{"b":{"b":1}}})", R"({"a":1,"b":{"b":{"b":1}},"c":1})"},
        NotSortedTestData{R"({"b":{"b":1},"a":{"a":1}})", R"({"a":{"a":1},"b":{"b":1}})"},
        NotSortedTestData{
            R"({"c":{"b":1,"a":1},"b":{"b":1,"a":1},"a":1})",
            R"({"a":1,"b":{"a":1,"b":1},"c":{"a":1,"b":1}})"
        },
        NotSortedTestData{R"({"b":1,"c":{"c":{"c":1}},"a":1})", R"({"a":1,"b":1,"c":{"c":{"c":1}}})"},
        NotSortedTestData{R"({"b":{"b":{"b":1}},"a":{"a":1},"c":1})", R"({"a":{"a":1},"b":{"b":{"b":1}},"c":1})"},
        NotSortedTestData{R"({"c":1,"b":{"b":{"b":1}},"a":{"a":1}})", R"({"a":{"a":1},"b":{"b":{"b":1}},"c":1})"},
        NotSortedTestData{R"({"c":{"c":1},"a":1,"b":{"b":{"b":1}}})", R"({"a":1,"b":{"b":{"b":1}},"c":{"c":1}})"},
        NotSortedTestData{
            R"({"b":{"b":{"b":1}},"c":{"c":{"c":1}},"a":1})",
            R"({"a":1,"b":{"b":{"b":1}},"c":{"c":{"c":1}}})"
        },
        NotSortedTestData{R"({"b":{"b":{"b":1,"a":1}},"a":1,"c":1})", R"({"a":1,"b":{"b":{"a":1,"b":1}},"c":1})"},
        NotSortedTestData{R"({"c":1,"b":{"b":{"b":1,"a":1}},"a":1})", R"({"a":1,"b":{"b":{"a":1,"b":1}},"c":1})"}
    )
);

INSTANTIATE_TEST_SUITE_P(
    JsonToString,
    JsonToStringCycle,
    ::testing::Values(
        CycleTestData{"null"},
        CycleTestData{"false"},
        CycleTestData{"true"},
        CycleTestData{"{}"},
        CycleTestData{"[]"},
        CycleTestData{"1"},
        CycleTestData{R"({"b":1,"a":1})"},
        CycleTestData{R"({"b":1,"a":1,"c":1})"},
        CycleTestData{R"({"b":{"b":1,"a":1}, "a":1,"c":1})"},
        CycleTestData{R"({"b":1,"a":1,"c":{"b":1,"a":1}})"},
        CycleTestData{R"({"b":{"b":{"b":1}},"a":1,"c":1})"},
        CycleTestData{R"({"a":1,"c":1,"b":{"b":{"b":1}}})"},
        CycleTestData{R"({"b":{"b":1},"a":{"a":1}})"},
        CycleTestData{R"({"c":{"b":1,"a":1},"b":{"b":1,"a":1},"a":1})"},
        CycleTestData{R"({"b":1,"c":{"c":{"c":1}},"a":1})"},
        CycleTestData{R"({"b":{"b":{"b":1}},"a":{"a":1},"c":1})"},
        CycleTestData{R"({"c":1,"b":{"b":{"b":1}},"a":{"a":1}})"},
        CycleTestData{R"({"c":{"c":1},"a":1,"b":{"b":{"b":1}}})"},
        CycleTestData{R"({"b":{"b":{"b":1}},"c":{"c":{"c":1}},"a":1})"},
        CycleTestData{R"({"b":{"b":{"b":1,"a":1}},"a":1,"c":1})"},
        CycleTestData{R"({"c":1,"b":{"b":{"b":1,"a":1}},"a":1})"}
    )
);

TEST(JsonToSortedString, Object) {
    const formats::json::Value example = formats::json::FromString(R"({"D":{"C":2},"A":1,"B":"sample"})");
    ASSERT_EQ(formats::json::ToStableString(example), R"({"A":1,"B":"sample","D":{"C":2}})");
}

TEST(JsonToSortedString, KeysSortedLexicographically) {
    const formats::json::Value example =
        formats::json::FromString(R"({"Sz":1,"Sample":1,"Sam":1,"SampleTest":1,"A":1,"Z":1,"SampleA":1,"SampleZ":1})");
    ASSERT_EQ(
        formats::json::ToStableString(example),
        R"({"A":1,"Sam":1,"Sample":1,"SampleA":1,"SampleTest":1,"SampleZ":1,"Sz":1,"Z":1})"
    );
}

TEST(JsonToSortedString, ExceededJsonDepthLimit) {
    EXPECT_THROW(
        formats::json::FromString(R"({"key1":{"key2":{"key3":{"key4":{"key5":{"key6":{"key7":{"key8":{"key9":{"key10":{"key11":{"key12":{"key13":{"key14":{"key15":{"key16":{"key17":{"key18":{"key19":{"key20":{"key21":{"key22":{"key23":{"key24":{"key25":{"key26":{"key27":{"key28":{"key29":{"key30":{"key31":{"key32":{"key33":{"key34":{"key35":{"key36":{"key37":{"key38":{"key39":{"key40":{"key41":{"key42":{"key43":{"key44":{"key45":{"key46":{"key47":{"key48":{"key49":{"key50":{"key51":{"key52":{"key53":{"key54":{"key55":{"key56":{"key57":{"key58":{"key59":{"key60":{"key61":{"key62":{"key63":{"key64":{"key65":{"key66":{"key67":{"key68":{"key69":{"key70":{"key71":{"key72":{"key73":{"key74":{"key75":{"key76":{"key77":{"key78":{"key79":{"key80":{"key81":{"key82":{"key83":{"key84":{"key85":{"key86":{"key87":{"key88":{"key89":{"key90":{"key91":{"key92":{"key93":{"key94":{"key95":{"key96":{"key97":{"key98":{"key99":{"key100":{"key101":{"key102":{"key103":{"key104":{"key105":{"key106":{"key107":{"key108":{"key109":{"key110":{"key111":{"key112":{"key113":{"key114":{"key115":{"key116":{"key117":{"key118":{"key119":{"key120":{"key121":{"key122":{"key123":{"key124":{"key125":{"key126":{"key127":{"key128":1}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}})"
        ),
        formats::json::ParseException
    );
    try {
        formats::json::FromString(R"({"key1":{"key2":{"key3":{"key4":{"key5":{"key6":{"key7":{"key8":{"key9":{"key10":{"key11":{"key12":{"key13":{"key14":{"key15":{"key16":{"key17":{"key18":{"key19":{"key20":{"key21":{"key22":{"key23":{"key24":{"key25":{"key26":{"key27":{"key28":{"key29":{"key30":{"key31":{"key32":{"key33":{"key34":{"key35":{"key36":{"key37":{"key38":{"key39":{"key40":{"key41":{"key42":{"key43":{"key44":{"key45":{"key46":{"key47":{"key48":{"key49":{"key50":{"key51":{"key52":{"key53":{"key54":{"key55":{"key56":{"key57":{"key58":{"key59":{"key60":{"key61":{"key62":{"key63":{"key64":{"key65":{"key66":{"key67":{"key68":{"key69":{"key70":{"key71":{"key72":{"key73":{"key74":{"key75":{"key76":{"key77":{"key78":{"key79":{"key80":{"key81":{"key82":{"key83":{"key84":{"key85":{"key86":{"key87":{"key88":{"key89":{"key90":{"key91":{"key92":{"key93":{"key94":{"key95":{"key96":{"key97":{"key98":{"key99":{"key100":{"key101":{"key102":{"key103":{"key104":{"key105":{"key106":{"key107":{"key108":{"key109":{"key110":{"key111":{"key112":{"key113":{"key114":{"key115":{"key116":{"key117":{"key118":{"key119":{"key120":{"key121":{"key122":{"key123":{"key124":{"key125":{"key126":{"key127":{"key128":1}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}})"
        );
    } catch (const formats::json::ParseException& e) {
        EXPECT_EQ(std::string(e.what()), "Exceeded maximum allowed JSON depth of: 128");
    }
}

TEST(JsonToSortedString, DuplicatedKeys) {
    EXPECT_THROW(formats::json::FromString(R"({"Key1":1,"Key2":2,"Key3":3, "Key1":2})"), formats::json::ParseException);
    try {
        formats::json::FromString(R"({"Key1":1,"Key2":2,"Key3":3, "Key1":2})");
    } catch (const formats::json::ParseException& e) {
        EXPECT_EQ(std::string(e.what()), "Duplicate key: Key1 at /");
    }
}

TEST(JsonToSortedString, DuplicatedKeysInObject) {
    EXPECT_THROW(
        formats::json::FromString(R"({"Key1":{"Key4":1,"Key5":2,"Key4":1},"Key2":2,"Key3":3})"),
        formats::json::ParseException
    );
    try {
        formats::json::FromString(R"({"Key1":{"Key4":1,"Key5":2,"Key4":1},"Key2":2,"Key3":3})");
    } catch (const formats::json::ParseException& e) {
        EXPECT_EQ(std::string(e.what()), "Duplicate key: Key4 at Key1");
    }
}

TEST(JsonToSortedString, NestedObjects) {
    const formats::json::Value example = formats::json::FromString(R"({"B":{"F":3,"D":1,"E":2},"A":1,"C":3})");
    ASSERT_EQ(formats::json::ToStableString(example), R"({"A":1,"B":{"D":1,"E":2,"F":3},"C":3})");
}

TEST(JsonToSortedString, Array) {
    const formats::json::Value example = formats::json::FromString(R"({"A":[1,3,2]})");
    ASSERT_EQ(formats::json::ToStableString(example), R"({"A":[1,3,2]})");
}

TEST(JsonToSortedString, ObjectInArray) {
    const formats::json::Value example = formats::json::FromString(R"({"A":[1,3,{"D":1,"B":1,"C":1}]})");
    ASSERT_EQ(formats::json::ToStableString(example), R"({"A":[1,3,{"B":1,"C":1,"D":1}]})");
}

TEST(JsonToSortedString, ASCII) {
    ASSERT_EQ("\u0041", "A");
    const formats::json::Value escaped = formats::json::FromString(R"({"\u0041":1})");
    const formats::json::Value unescaped = formats::json::FromString(R"({"A":1})");
    ASSERT_EQ(formats::json::ToStableString(escaped), formats::json::ToStableString(unescaped));
}

TEST(JsonToSortedString, NonASCII) {
    ASSERT_EQ("\u5143", "元");
    const formats::json::Value escaped = formats::json::FromString(R"({"\u5143":1})");
    const formats::json::Value unescaped = formats::json::FromString(R"({"元":1})");
    ASSERT_EQ(formats::json::ToStableString(escaped), formats::json::ToStableString(unescaped));
}

TEST(JsonToPrettyStringCycle, IsPretty) {
    static constexpr std::string_view kInitialJson = R"({"a":1,"b":[],"c":{},"d":{"x":[42]},"e":[5,"foo"]})";

    static constexpr std::string_view kPrettyJson = R"({
   "a": 1,
   "b": [],
   "c": {},
   "d": {
      "x": [
         42
      ]
   },
   "e": [
      5,
      "foo"
   ]
})";

    const auto json = formats::json::FromString(kInitialJson);

    formats::json::PrettyFormat format;
    format.indent_char_count = 3;
    EXPECT_EQ(kPrettyJson, formats::json::ToPrettyString(json, format));
}

// TODO make ToPrettyString sort object keys and re-enable.
TEST(JsonToPrettyStringCycle, DISABLED_SortsObjectKeys) {
    static constexpr std::string_view kInitialJson = R"({"c":1,"b":1,"a":1})";

    static constexpr std::string_view kPrettyJson = R"({
  "a": 1,
  "b": 1,
  "c": 1,
})";

    const auto json = formats::json::FromString(kInitialJson);
    EXPECT_EQ(kPrettyJson, formats::json::ToPrettyString(json));
}

template <typename T, auto... Validators>
auto FromStreamAs(const std::string& input) {
    std::istringstream stream(input);
    return formats::json::FromStreamAs<T, Validators...>(stream);
}

TEST(JsonAsTParser, BasicTypes) {
    // Test int
    EXPECT_EQ(formats::json::FromStringAs<int>("42"), 42);
    EXPECT_EQ(formats::json::FromStringAs<int>("-123"), -123);

    EXPECT_EQ(FromStreamAs<int>("42"), 42);
    EXPECT_EQ(FromStreamAs<int>("-123"), -123);

    // Test double
    EXPECT_DOUBLE_EQ(formats::json::FromStringAs<double>("3.14"), 3.14);
    EXPECT_DOUBLE_EQ(formats::json::FromStringAs<double>("-2.5"), -2.5);

    EXPECT_DOUBLE_EQ(FromStreamAs<double>("3.14"), 3.14);
    EXPECT_DOUBLE_EQ(FromStreamAs<double>("-2.5"), -2.5);

    // Test bool
    EXPECT_EQ(formats::json::FromStringAs<bool>("true"), true);
    EXPECT_EQ(formats::json::FromStringAs<bool>("false"), false);

    EXPECT_EQ(FromStreamAs<bool>("true"), true);
    EXPECT_EQ(FromStreamAs<bool>("false"), false);

    // Test string
    EXPECT_EQ(formats::json::FromStringAs<std::string>("\"hello\""), "hello");
    EXPECT_EQ(formats::json::FromStringAs<std::string>("\"test string\""), "test string");

    EXPECT_EQ(FromStreamAs<std::string>("\"hello\""), "hello");
    EXPECT_EQ(FromStreamAs<std::string>("\"test string\""), "test string");
}

TEST(JsonAsTParser, ArrayBool) {
    const std::string input{"[true, false, true]"};

    auto string_result = formats::json::FromStringAs<std::vector<bool>>(input);
    EXPECT_EQ(string_result, (std::vector<bool>{true, false, true}));

    auto stream_result = FromStreamAs<std::vector<bool>>(input);
    EXPECT_EQ(stream_result, (std::vector<bool>{true, false, true}));
}

TEST(JsonAsTParser, ArrayInt) {
    const std::string input = "[1, 2, 3, 4, 5]";

    auto string_result = formats::json::FromStringAs<std::vector<int>>(input);
    EXPECT_EQ(string_result, (std::vector<int>{1, 2, 3, 4, 5}));

    auto stream_result = FromStreamAs<std::vector<int>>(input);
    EXPECT_EQ(stream_result, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(JsonAsTParser, ArrayString) {
    const std::string input = R"(["apple", "banana", "cherry"])";

    auto string_result = formats::json::FromStringAs<std::vector<std::string>>(input);
    EXPECT_EQ(string_result, (std::vector<std::string>{"apple", "banana", "cherry"}));

    auto stream_result = FromStreamAs<std::vector<std::string>>(input);
    EXPECT_EQ(stream_result, (std::vector<std::string>{"apple", "banana", "cherry"}));
}

TEST(JsonAsTParser, ArrayDouble) {
    const std::string input = "[1.1, 2.2, 3.3]";

    auto string_result = formats::json::FromStringAs<std::vector<double>>(input);
    EXPECT_EQ(string_result, (std::vector<double>{1.1, 2.2, 3.3}));

    auto stream_result = FromStreamAs<std::vector<double>>(input);
    EXPECT_EQ(stream_result, (std::vector<double>{1.1, 2.2, 3.3}));
}

TEST(JsonAsTParser, MapStringInt) {
    const std::string input = R"({"one": 1, "two": 2, "three": 3})";

    std::map<std::string, int> expected{{"one", 1}, {"two", 2}, {"three", 3}};

    auto string_result = formats::json::FromStringAs<std::map<std::string, int>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, int>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, MapStringString) {
    const std::string input = R"({"name": "John", "city": "New York"})";

    std::map<std::string, std::string> expected{{"name", "John"}, {"city", "New York"}};

    auto string_result = formats::json::FromStringAs<std::map<std::string, std::string>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, std::string>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, MapStringBool) {
    const std::string input = R"({"active": true, "verified": false})";

    std::map<std::string, bool> expected{{"active", true}, {"verified", false}};

    auto string_result = formats::json::FromStringAs<std::map<std::string, bool>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, bool>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, UnorderedMapStringInt) {
    const std::string input = R"({"x": 10, "y": 20, "z": 30})";

    std::unordered_map<std::string, int> expected{{"x", 10}, {"y", 20}, {"z", 30}};

    auto string_result = formats::json::FromStringAs<std::unordered_map<std::string, int>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::unordered_map<std::string, int>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, ArrayOfMapsStringInt) {
    const std::string input = R"([{"id": 1, "count": 10}, {"id": 2, "count": 20}])";

    std::vector<std::map<std::string, int>> expected{{{"id", 1}, {"count", 10}}, {{"id", 2}, {"count", 20}}};

    auto string_result = formats::json::FromStringAs<std::vector<std::map<std::string, int>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<std::map<std::string, int>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, ArrayOfMapsStringString) {
    const std::string input = R"([{"name": "Alice", "role": "admin"}, {"name": "Bob", "role": "user"}])";

    std::vector<std::map<std::string, std::string>> expected{
        {{"name", "Alice"}, {"role", "admin"}}, {{"name", "Bob"}, {"role", "user"}}
    };

    auto string_result = formats::json::FromStringAs<std::vector<std::map<std::string, std::string>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<std::map<std::string, std::string>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, MapOfArraysInt) {
    const std::string input = R"({"numbers": [1, 2, 3], "scores": [10, 20, 30]})";

    std::map<std::string, std::vector<int>> expected{{"numbers", {1, 2, 3}}, {"scores", {10, 20, 30}}};

    auto string_result = formats::json::FromStringAs<std::map<std::string, std::vector<int>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, std::vector<int>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, MapOfArraysString) {
    const std::string input = R"({"fruits": ["apple", "banana"], "colors": ["red", "blue"]})";

    std::map<std::string, std::vector<std::string>> expected{
        {"fruits", {"apple", "banana"}}, {"colors", {"red", "blue"}}
    };

    auto string_result = formats::json::FromStringAs<std::map<std::string, std::vector<std::string>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, std::vector<std::string>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, NestedArrayInt) {
    const std::string input = "[[1, 2], [3, 4], [5, 6]]";

    std::vector<std::vector<int>> expected{{1, 2}, {3, 4}, {5, 6}};

    auto string_result = formats::json::FromStringAs<std::vector<std::vector<int>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<std::vector<int>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, NestedArrayString) {
    const std::string input = R"([["a", "b"], ["c", "d"], ["e", "f"]])";

    std::vector<std::vector<std::string>> expected{{"a", "b"}, {"c", "d"}, {"e", "f"}};

    auto string_result = formats::json::FromStringAs<std::vector<std::vector<std::string>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<std::vector<std::string>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, SetTypes) {
    const std::string input = R"(["apple", "banana", "apple", "cherry"])";

    std::set<std::string> set_expected{"apple", "banana", "cherry"};

    auto string_set_result = formats::json::FromStringAs<std::set<std::string>>(input);
    EXPECT_EQ(string_set_result, set_expected);

    auto stream_set_result = FromStreamAs<std::set<std::string>>(input);
    EXPECT_EQ(stream_set_result, set_expected);

    std::unordered_set<std::string> uset_expected{"apple", "banana", "cherry"};

    auto string_uset_result = formats::json::FromStringAs<std::unordered_set<std::string>>(input);
    EXPECT_EQ(string_uset_result, uset_expected);

    auto stream_uset_result = FromStreamAs<std::unordered_set<std::string>>(input);
    EXPECT_EQ(stream_uset_result, uset_expected);
}

TEST(JsonAsTParser, EmptyContainers) {
    // Empty array
    auto string_empty_vec = formats::json::FromStringAs<std::vector<int>>("[]");
    EXPECT_TRUE(string_empty_vec.empty());

    auto stream_empty_vec = FromStreamAs<std::vector<int>>("[]");
    EXPECT_TRUE(stream_empty_vec.empty());

    // Empty object
    auto string_empty_map = formats::json::FromStringAs<std::map<std::string, int>>("{}");
    EXPECT_TRUE(string_empty_map.empty());

    auto stream_empty_map = FromStreamAs<std::map<std::string, int>>("{}");
    EXPECT_TRUE(stream_empty_map.empty());

    // Empty set
    auto string_empty_set = formats::json::FromStringAs<std::set<std::string>>("[]");
    EXPECT_TRUE(string_empty_set.empty());

    auto stream_empty_set = FromStreamAs<std::set<std::string>>("[]");
    EXPECT_TRUE(stream_empty_set.empty());
}

TEST(JsonAsTParser, ErrorCases) {
    using JsonParserError = formats::json::parser::ParseError;

    // Invalid type conversion
    EXPECT_THROW((formats::json::FromStringAs<int>("\"not_a_number\"")), JsonParserError);
    EXPECT_THROW((FromStreamAs<int>("\"not_a_number\"")), JsonParserError);

    // Missing field in object
    EXPECT_THROW((formats::json::FromStringAs<std::map<std::string, int>>("{\"a\": 1, \"b\":}")), JsonParserError);
    EXPECT_THROW((FromStreamAs<std::map<std::string, int>>("{\"a\": 1, \"b\":}")), JsonParserError);

    // Malformed JSON
    EXPECT_THROW((formats::json::FromStringAs<std::vector<int>>("[1, 2, ")), JsonParserError);
    EXPECT_THROW((FromStreamAs<std::vector<int>>("[1, 2, ")), JsonParserError);

    // Type mismatch in array
    EXPECT_THROW((formats::json::FromStringAs<std::vector<int>>("[1, \"two\", 3]")), JsonParserError);
    EXPECT_THROW((FromStreamAs<std::vector<int>>("[1, \"two\", 3]")), JsonParserError);

    // Type mismatch in map
    EXPECT_THROW(
        (formats::json::FromStringAs<std::map<std::string, int>>("{\"a\": 1, \"b\": \"text\"}")), JsonParserError
    );
    EXPECT_THROW((FromStreamAs<std::map<std::string, int>>("{\"a\": 1, \"b\": \"text\"}")), JsonParserError);
}

struct CustomType {
    int x;
    int y;

    bool operator==(const CustomType& other) const { return x == other.x && y == other.y; }
};

CustomType Parse(const formats::json::Value& value, formats::parse::To<CustomType>) {
    return CustomType{value["x"].As<int>(), value["y"].As<int>()};
}

TEST(JsonAsTParser, CustomTypeSimple) {
    const std::string input = R"({"x": 10, "y": 20})";

    CustomType expected{10, 20};

    auto string_result = formats::json::FromStringAs<CustomType>(input);
    EXPECT_EQ(string_result.x, expected.x);
    EXPECT_EQ(string_result.y, expected.y);

    auto stream_result = FromStreamAs<CustomType>(input);
    EXPECT_EQ(stream_result.x, expected.x);
    EXPECT_EQ(stream_result.y, expected.y);
}

TEST(JsonAsTParser, CustomTypeInVector) {
    const std::string input = R"([{"x": 1, "y": 2}, {"x": 3, "y": 4}])";

    std::vector<CustomType> expected{{1, 2}, {3, 4}};

    auto string_result = formats::json::FromStringAs<std::vector<CustomType>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<CustomType>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, CustomTypeInMap) {
    const std::string input = R"({"point1": {"x": 1, "y": 2}, "point2": {"x": 3, "y": 4}})";

    std::map<std::string, CustomType> expected{{"point1", {1, 2}}, {"point2", {3, 4}}};

    auto string_result = formats::json::FromStringAs<std::map<std::string, CustomType>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, CustomType>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, VectorOfCustomTypeInMap) {
    const std::string input = R"({"line1": [{"x": 1, "y": 2}, {"x": 3, "y": 4}], "line2": [{"x": 5, "y": 6}]})";

    std::map<std::string, std::vector<CustomType>> expected{{"line1", {{1, 2}, {3, 4}}}, {"line2", {{5, 6}}}};

    auto string_result = formats::json::FromStringAs<std::map<std::string, std::vector<CustomType>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, std::vector<CustomType>>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, MapInVectorOfCustomType) {
    const std::string input = R"([{"start": {"x": 1, "y": 2}, "end": {"x": 3, "y": 4}}])";

    std::vector<std::map<std::string, CustomType>> expected{{{"start", {1, 2}}, {"end", {3, 4}}}};

    auto string_result = formats::json::FromStringAs<std::vector<std::map<std::string, CustomType>>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<std::map<std::string, CustomType>>>(input);
    EXPECT_EQ(stream_result, expected);
}

struct ComplexType {
    int id;
    std::string name;
    bool active;
    std::vector<int> numbers;
    std::map<std::string, std::string> properties;
    std::vector<CustomType> points;
    std::unordered_map<std::string, bool> flags;

    bool operator==(const ComplexType& other) const {
        return id == other.id && name == other.name && active == other.active && numbers == other.numbers &&
               properties == other.properties && points == other.points && flags == other.flags;
    }
};

ComplexType Parse(const formats::json::Value& value, formats::parse::To<ComplexType>) {
    return ComplexType{
        value["id"].As<int>(),
        value["name"].As<std::string>(),
        value["active"].As<bool>(),
        value["numbers"].As<std::vector<int>>(),
        value["properties"].As<std::map<std::string, std::string>>(),
        value["points"].As<std::vector<CustomType>>(),
        value["flags"].As<std::unordered_map<std::string, bool>>()
    };
}

TEST(JsonAsTParser, ComplexType) {
    const std::string input = R"({
        "id": 42,
        "name": "test object",
        "active": true,
        "numbers": [1, 2, 3, 4, 5],
        "properties": {
            "color": "red",
            "size": "large",
            "weight": "heavy"
        },
        "points": [
            {"x": 1, "y": 2},
            {"x": 3, "y": 4},
            {"x": 5, "y": 6}
        ],
        "flags": {
            "feature1": true,
            "feature2": false,
            "feature3": true
        }
    })";

    ComplexType expected{
        42,
        "test object",
        true,
        {1, 2, 3, 4, 5},
        {{"color", "red"}, {"size", "large"}, {"weight", "heavy"}},
        {{1, 2}, {3, 4}, {5, 6}},
        {{"feature1", true}, {"feature2", false}, {"feature3", true}}
    };

    auto string_result = formats::json::FromStringAs<ComplexType>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<ComplexType>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, VectorOfComplexTypes) {
    const std::string input = R"([
        {
            "id": 1,
            "name": "first",
            "active": true,
            "numbers": [1, 2],
            "properties": {"key1": "value1"},
            "points": [{"x": 1, "y": 1}],
            "flags": {"flag1": true}
        },
        {
            "id": 2,
            "name": "second",
            "active": false,
            "numbers": [3, 4],
            "properties": {"key2": "value2"},
            "points": [{"x": 2, "y": 2}],
            "flags": {"flag2": false}
        }
    ])";

    std::vector<ComplexType> expected{
        {1, "first", true, {1, 2}, {{"key1", "value1"}}, {{1, 1}}, {{"flag1", true}}},
        {2, "second", false, {3, 4}, {{"key2", "value2"}}, {{2, 2}}, {{"flag2", false}}}
    };

    auto string_result = formats::json::FromStringAs<std::vector<ComplexType>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::vector<ComplexType>>(input);
    EXPECT_EQ(stream_result, expected);
}

TEST(JsonAsTParser, MapOfComplexTypes) {
    const std::string input = R"({
        "item1": {
            "id": 1,
            "name": "first item",
            "active": true,
            "numbers": [10, 20],
            "properties": {"type": "primary"},
            "points": [{"x": 10, "y": 20}],
            "flags": {"enabled": true}
        },
        "item2": {
            "id": 2,
            "name": "second item",
            "active": false,
            "numbers": [30, 40],
            "properties": {"type": "secondary"},
            "points": [{"x": 30, "y": 40}],
            "flags": {"enabled": false}
        }
    })";

    std::map<std::string, ComplexType> expected{
        {"item1", {1, "first item", true, {10, 20}, {{"type", "primary"}}, {{10, 20}}, {{"enabled", true}}}},
        {"item2", {2, "second item", false, {30, 40}, {{"type", "secondary"}}, {{30, 40}}, {{"enabled", false}}}}
    };

    auto string_result = formats::json::FromStringAs<std::map<std::string, ComplexType>>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<std::map<std::string, ComplexType>>(input);
    EXPECT_EQ(stream_result, expected);
}

struct NestedComplexType {
    ComplexType main;
    std::vector<ComplexType> alternatives;
    std::map<std::string, ComplexType> variants;

    bool operator==(const NestedComplexType& other) const {
        return main == other.main && alternatives == other.alternatives && variants == other.variants;
    }
};

NestedComplexType Parse(const formats::json::Value& value, formats::parse::To<NestedComplexType>) {
    return NestedComplexType{
        value["main"].As<ComplexType>(),
        value["alternatives"].As<std::vector<ComplexType>>(),
        value["variants"].As<std::map<std::string, ComplexType>>()
    };
}

TEST(JsonAsTParser, NestedComplexType) {
    const std::string input = R"({
        "main": {
            "id": 1,
            "name": "main object",
            "active": true,
            "numbers": [1, 2, 3],
            "properties": {"main": "true"},
            "points": [{"x": 1, "y": 1}],
            "flags": {"main_flag": true}
        },
        "alternatives": [
            {
                "id": 2,
                "name": "alt1",
                "active": false,
                "numbers": [4, 5],
                "properties": {"alt": "1"},
                "points": [{"x": 2, "y": 2}],
                "flags": {"alt_flag": false}
            },
            {
                "id": 3,
                "name": "alt2",
                "active": true,
                "numbers": [6, 7],
                "properties": {"alt": "2"},
                "points": [{"x": 3, "y": 3}],
                "flags": {"alt_flag": true}
            }
        ],
        "variants": {
            "var1": {
                "id": 4,
                "name": "variant1",
                "active": true,
                "numbers": [8, 9],
                "properties": {"var": "1"},
                "points": [{"x": 4, "y": 4}],
                "flags": {"var_flag": true}
            },
            "var2": {
                "id": 5,
                "name": "variant2",
                "active": false,
                "numbers": [10, 11],
                "properties": {"var": "2"},
                "points": [{"x": 5, "y": 5}],
                "flags": {"var_flag": false}
            }
        }
    })";

    NestedComplexType expected{
        {1, "main object", true, {1, 2, 3}, {{"main", "true"}}, {{1, 1}}, {{"main_flag", true}}},
        {{2, "alt1", false, {4, 5}, {{"alt", "1"}}, {{2, 2}}, {{"alt_flag", false}}},
         {3, "alt2", true, {6, 7}, {{"alt", "2"}}, {{3, 3}}, {{"alt_flag", true}}}},
        {{"var1", {4, "variant1", true, {8, 9}, {{"var", "1"}}, {{4, 4}}, {{"var_flag", true}}}},
         {"var2", {5, "variant2", false, {10, 11}, {{"var", "2"}}, {{5, 5}}, {{"var_flag", false}}}}}
    };

    auto string_result = formats::json::FromStringAs<NestedComplexType>(input);
    EXPECT_EQ(string_result, expected);

    auto stream_result = FromStreamAs<NestedComplexType>(input);
    EXPECT_EQ(stream_result, expected);
}

void ValidatePositive(int value) {
    if (value <= 0) {
        throw std::runtime_error("Value must be positive");
    }
}

void ValidateRange(int value) {
    if (value < 1 || value > 100) {
        throw std::runtime_error("Value must be between 1 and 100");
    }
}

void ValidateNonEmpty(const std::vector<int>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector must not be empty");
    }
}

void ValidateSize(const std::vector<int>& vec) {
    if (vec.size() > 5) {
        throw std::runtime_error("Vector size must not exceed 5");
    }
}

void ValidateUnorderedMapSize(const std::unordered_map<std::string, int>& u_map) {
    if (u_map.size() > 5) {
        throw std::runtime_error("Vector size must not exceed 5");
    }
}

void ValidateMapSize(const std::map<std::string, int>& map) {
    if (map.size() > 5) {
        throw std::runtime_error("Vector size must not exceed 5");
    }
}

void ValidateSetSize(const std::set<std::string>& set) {
    if (set.size() > 5) {
        throw std::runtime_error("Vector size must not exceed 5");
    }
}

void ValidateUnorderedSetSize(const std::unordered_set<std::string>& u_set) {
    if (u_set.size() > 5) {
        throw std::runtime_error("Vector size must not exceed 5");
    }
}

void ValidateStringLength(const std::string& str) {
    if (str.length() > 10) {
        throw std::runtime_error("String is too long");
    }
}

void ValidateCustomType(const CustomType& val) {
    if (val.x < 0 || val.y < 0) {
        throw std::runtime_error("CustomType coordinates must be non-negative");
    }
}

void ValidatorOrderA(int) { throw std::runtime_error("Validator A failed"); }
void ValidatorOrderB(int) { throw std::runtime_error("Validator B failed"); }

void ValidateIsEven(int value) {
    if (value % 2 != 0) {
        throw std::runtime_error("Value must be even");
    }
}

void ValidateNoCyrillic(const std::string& str) {
    for (unsigned char c : str) {
        if (c >= 0x80) {
            throw std::runtime_error("String must not contain non-ASCII characters");
        }
    }
}

void ValidatePointsVector(const std::vector<CustomType>& points) {
    if (points.size() < 2) {
        throw std::runtime_error("There must be at least two points");
    }
}

void ValidateInnerMapSize(const std::unordered_map<std::string, int>& map) {
    if (map.size() < 1) {
        throw std::runtime_error("Inner map cannot be empty");
    }
}
void ValidateOuterVectorSize(const std::vector<std::unordered_map<std::string, int>>& vec) {
    if (vec.size() > 2) {
        throw std::runtime_error("Outer vector size cannot exceed 2");
    }
}

void ValidateVecMapSize(const std::vector<std::unordered_map<std::string, int>>& vec) {
    if (vec.size() > 5) {
        throw std::runtime_error("Vector of maps size must not exceed 5");
    }
}

void ValidateSetOfInts(const std::set<int>& s) {
    if (s.empty()) {
        throw std::runtime_error("Inner set must not be empty");
    }
}

void ValidateVectorOfSets(const std::vector<std::set<int>>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector of sets must not be empty");
    }
    if (vec.size() > 3) {
        throw std::runtime_error("Vector of sets must not have more than 3 sets");
    }
}

void ValidateInnerMap(const std::unordered_map<std::string, std::vector<std::set<int>>>& map) {
    if (map.empty()) {
        throw std::runtime_error("Inner map must not be empty");
    }
}

void ValidateOuterVector(const std::vector<std::unordered_map<std::string, std::vector<std::set<int>>>>& vec) {
    if (vec.size() > 2) {
        throw std::runtime_error("Outer vector must not have more than 2 elements");
    }
}

TEST(JsonAsTParser, WithValidatorsInt) {
    using namespace formats::json;

    EXPECT_EQ((FromStringAs<int, ValidatePositive, ValidateRange>("42")), 42);
    EXPECT_THROW((FromStringAs<int, ValidatePositive, ValidateRange>("-5")), parser::ParseError);
    EXPECT_THROW((FromStringAs<int, ValidatePositive, ValidateRange>("150")), parser::ParseError);
    EXPECT_THROW((FromStringAs<int, ValidatePositive, ValidateRange>("0")), parser::ParseError);

    EXPECT_EQ((FromStreamAs<int, ValidatePositive, ValidateRange>("42")), 42);
    EXPECT_THROW((FromStreamAs<int, ValidatePositive, ValidateRange>("-5")), parser::ParseError);
    EXPECT_THROW((FromStreamAs<int, ValidatePositive, ValidateRange>("150")), parser::ParseError);
    EXPECT_THROW((FromStreamAs<int, ValidatePositive, ValidateRange>("0")), parser::ParseError);
}

TEST(JsonAsTParser, WithValidatorsVector) {
    using namespace formats::json;

    const std::vector<int> expected{1, 2, 3};

    EXPECT_EQ((FromStringAs<std::vector<int>, ValidateNonEmpty, ValidateSize>("[1, 2, 3]")), expected);
    EXPECT_THROW((FromStringAs<std::vector<int>, ValidateNonEmpty, ValidateSize>("[]")), parser::ParseError);
    EXPECT_THROW(
        (FromStringAs<std::vector<int>, ValidateNonEmpty, ValidateSize>("[1, 2, 3, 4, 5, 6]")), parser::ParseError
    );

    EXPECT_EQ((FromStreamAs<std::vector<int>, ValidateNonEmpty, ValidateSize>("[1, 2, 3]")), expected);
    EXPECT_THROW((FromStreamAs<std::vector<int>, ValidateNonEmpty, ValidateSize>("[]")), parser::ParseError);
    EXPECT_THROW(
        (FromStreamAs<std::vector<int>, ValidateNonEmpty, ValidateSize>("[1, 2, 3, 4, 5, 6]")), parser::ParseError
    );
}

TEST(JsonAsTParser, WithValidators_SingleValidator) {
    using namespace formats::json;

    EXPECT_EQ((FromStringAs<int, ValidatePositive>("150")), 150);
    EXPECT_THROW((FromStringAs<int, ValidatePositive>("-5")), parser::ParseError);

    EXPECT_EQ((FromStreamAs<int, ValidatePositive>("150")), 150);
    EXPECT_THROW((FromStreamAs<int, ValidatePositive>("-5")), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_ExecutionOrder) {
    using namespace formats::json;

    try {
        FromStringAs<int, ValidatorOrderA, ValidatorOrderB>("0");
        FAIL() << "Exception was not thrown";
    } catch (const parser::ParseError& e) {
        std::string_view message(e.what());
        EXPECT_TRUE(message.find("Validator A failed") != std::string_view::npos)
            << "The error message did not contain 'Validator A failed'. Actual message: " << message;
    }

    try {
        FromStringAs<int, ValidatorOrderB, ValidatorOrderA>("0");
        FAIL() << "Exception was not thrown";
    } catch (const parser::ParseError& e) {
        std::string_view message(e.what());
        EXPECT_TRUE(message.find("Validator B failed") != std::string_view::npos)
            << "The error message did not contain 'Validator B failed'. Actual message: " << message;
    }
}

TEST(JsonAsTParser, WithValidators_String) {
    using namespace formats::json;
    const std::string short_str = "short";
    const std::string long_str = "this is a very long string";

    EXPECT_EQ((FromStringAs<std::string, ValidateStringLength>(fmt::format("\"{}\"", short_str))), short_str);
    EXPECT_THROW(
        (FromStringAs<std::string, ValidateStringLength>(fmt::format("\"{}\"", long_str))), parser::ParseError
    );

    EXPECT_EQ((FromStreamAs<std::string, ValidateStringLength>(fmt::format("\"{}\"", short_str))), short_str);
    EXPECT_THROW(
        (FromStreamAs<std::string, ValidateStringLength>(fmt::format("\"{}\"", long_str))), parser::ParseError
    );
}

TEST(JsonAsTParser, WithValidators_CustomType) {
    using namespace formats::json;
    const std::string good_json = R"({"x": 10, "y": 20})";
    const std::string bad_json = R"({"x": -1, "y": 20})";

    EXPECT_NO_THROW((FromStringAs<CustomType, ValidateCustomType>(good_json)));
    EXPECT_THROW((FromStringAs<CustomType, ValidateCustomType>(bad_json)), parser::ParseError);

    EXPECT_NO_THROW((FromStreamAs<CustomType, ValidateCustomType>(good_json)));
    EXPECT_THROW((FromStreamAs<CustomType, ValidateCustomType>(bad_json)), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_MultipleValidatorsSecondFails) {
    using namespace formats::json;

    EXPECT_EQ((FromStringAs<int, ValidateRange, ValidateIsEven>("50")), 50);

    try {
        FromStringAs<int, ValidateRange, ValidateIsEven>("51");
        FAIL() << "Exception was not thrown";
    } catch (const parser::ParseError& e) {
        std::string_view message(e.what());
        EXPECT_TRUE(message.find("Value must be even") != std::string_view::npos);
        EXPECT_TRUE(message.find("Value must be between 1 and 100") == std::string_view::npos);
    }
}

TEST(JsonAsTParser, WithValidators_Utf8Strings) {
    using namespace formats::json;

    EXPECT_EQ((FromStringAs<std::string, ValidateNoCyrillic>("\"Hello World\"")), "Hello World");

    EXPECT_THROW((FromStringAs<std::string, ValidateNoCyrillic>("\"Привет, мир\"")), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_ParsingErrorBeforeValidation) {
    using namespace formats::json;

    try {
        FromStringAs<int, ValidatePositive>("null");
        FAIL() << "Exception was not thrown";
    } catch (const parser::ParseError& e) {
        std::string_view message(e.what());

        const std::string expected_substring = "integer was expected, but null found";
        EXPECT_TRUE(message.find(expected_substring) != std::string_view::npos)
            << "The error message did not contain '" << expected_substring << "'. Actual message: " << message;
    }
}

TEST(JsonAsTParser, WithValidators_VectorOfCustomType) {
    using namespace formats::json;

    const std::string good_json = R"([{"x": 1, "y": 2}, {"x": 3, "y": 4}])";
    EXPECT_NO_THROW((FromStringAs<std::vector<CustomType>, ValidatePointsVector>(good_json)));

    const std::string bad_json = R"([{"x": 1, "y": 2}])";
    EXPECT_THROW((FromStringAs<std::vector<CustomType>, ValidatePointsVector>(bad_json)), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_Hierarchical_Elements) {
    using namespace formats::json;

    using ValidatedVector = std::vector<parser::WithValidators<int, ValidatePositive>>;
    const std::vector<int> expected{10, 20, 30};

    EXPECT_EQ((FromStringAs<ValidatedVector>("[10, 20, 30]")), expected);
    EXPECT_EQ((FromStreamAs<ValidatedVector>("[10, 20, 30]")), expected);

    EXPECT_THROW((FromStringAs<ValidatedVector>("[10, -20, 30]")), parser::ParseError);
    EXPECT_THROW((FromStreamAs<ValidatedVector>("[10, -20, 30]")), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_Hierarchical_Mixed) {
    using namespace formats::json;

    using ValidatedVector = std::vector<parser::WithValidators<int, ValidatePositive>>;
    const std::vector<int> expected{1, 2, 3};

    EXPECT_EQ((FromStringAs<ValidatedVector, ValidateNonEmpty, ValidateSize>("[1, 2, 3]")), expected);

    EXPECT_THROW((FromStringAs<ValidatedVector, ValidateNonEmpty, ValidateSize>("[1, -2, 3]")), parser::ParseError);

    EXPECT_THROW((FromStringAs<ValidatedVector, ValidateNonEmpty, ValidateSize>("[]")), parser::ParseError);

    EXPECT_THROW(
        (FromStringAs<ValidatedVector, ValidateNonEmpty, ValidateSize>("[1, 2, 3, 4, 5, 6]")), parser::ParseError
    );
}

TEST(JsonAsTParser, WithValidators_MapKeysAndValues) {
    using namespace formats::json;

    using ValidatedString = parser::WithValidators<std::string, ValidateStringLength>;
    using ValidatedInt = parser::WithValidators<int, ValidatePositive>;
    using ValidatedKeyValue = std::unordered_map<ValidatedString, ValidatedInt>;

    const std::unordered_map<std::string, int> expected{{"first", 1}, {"second", 2}};

    EXPECT_EQ((FromStringAs<ValidatedKeyValue, ValidateUnorderedMapSize>(R"({"first": 1, "second": 2})")), expected);

    const std::string bad_value_json = R"({"first": 1, "second": -2})";
    EXPECT_THROW((FromStringAs<ValidatedKeyValue, ValidateUnorderedMapSize>(bad_value_json)), parser::ParseError);

    const std::string large_map_json = R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6})";
    EXPECT_THROW((FromStringAs<ValidatedKeyValue, ValidateUnorderedMapSize>(large_map_json)), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_OtherContainers) {
    using namespace formats::json;

    using ValidatedSet = std::set<parser::WithValidators<std::string, ValidateStringLength>>;
    const std::set<std::string> expected_set{"a", "b", "c"};

    EXPECT_EQ((FromStringAs<ValidatedSet, ValidateSetSize>(R"(["a", "b", "c"])")), expected_set);
    EXPECT_THROW(
        (FromStringAs<ValidatedSet, ValidateSetSize>(R"(["a", "b", "string is too long"])")), parser::ParseError
    );
    EXPECT_THROW(
        (FromStringAs<ValidatedSet, ValidateSetSize>(R"(["a", "b", "c", "d", "e", "f"])")), parser::ParseError
    );

    using ValidatedUnorderedSet = std::unordered_set<parser::WithValidators<std::string, ValidateStringLength>>;
    const std::unordered_set<std::string> expected_u_set{"a", "b", "c"};

    EXPECT_EQ(
        (FromStringAs<ValidatedUnorderedSet, ValidateUnorderedSetSize>(R"(["c", "a", "b", "a"])")), expected_u_set
    );

    EXPECT_THROW(
        (FromStringAs<ValidatedUnorderedSet, ValidateUnorderedSetSize>(R"(["a", "b", "string is too long"])")),
        parser::ParseError
    );

    EXPECT_THROW(
        (FromStringAs<ValidatedUnorderedSet, ValidateUnorderedSetSize>(R"(["a", "b", "c", "d", "e", "f"])")),
        parser::ParseError
    );
}

TEST(JsonAsTParser, WithValidators_DeeplyNestedMixed) {
    using namespace formats::json;

    using ValidatedInt = parser::WithValidators<int, ValidatePositive>;
    using InnerMap = std::unordered_map<std::string, ValidatedInt>;

    using ValidatedInnerMap = parser::WithValidators<InnerMap, ValidateInnerMapSize>;
    using OuterVector = std::vector<ValidatedInnerMap>;

    const std::vector<std::unordered_map<std::string, int>> expected = {{{"a", 1}, {"b", 2}}, {{"c", 3}}};
    const std::string good_json = R"([{"a": 1, "b": 2}, {"c": 3}])";
    EXPECT_EQ((FromStringAs<OuterVector, ValidateOuterVectorSize>(good_json)), expected);

    const std::string bad_int_json = R"([{"a": 1, "b": -2}])";
    EXPECT_THROW((FromStringAs<OuterVector, ValidateOuterVectorSize>(bad_int_json)), parser::ParseError);

    const std::string bad_map_json = R"([{"a": 1}, {}])";
    EXPECT_THROW((FromStringAs<OuterVector, ValidateOuterVectorSize>(bad_map_json)), parser::ParseError);

    const std::string bad_vec_json = R"([{"a": 1}, {"b": 2}, {"c": 3}])";
    EXPECT_THROW((FromStringAs<OuterVector, ValidateOuterVectorSize>(bad_vec_json)), parser::ParseError);
}

TEST(JsonAsTParser, WithValidators_StdMap) {
    using namespace formats::json;

    using ValidatedMap = std::map<std::string, parser::WithValidators<int, ValidatePositive>>;

    const std::map<std::string, int> expected_map{{"alpha", 1}, {"beta", 2}};

    const std::string good_json = R"({"beta": 2, "alpha": 1})";
    EXPECT_EQ((FromStringAs<ValidatedMap, ValidateMapSize>(good_json)), expected_map);

    const std::string bad_value_json = R"({"alpha": 1, "beta": -5})";
    EXPECT_THROW((FromStringAs<ValidatedMap, ValidateMapSize>(bad_value_json)), parser::ParseError);

    const std::string large_map_json = R"({"a":1, "b":2, "c":3, "d":4, "e":5, "f":6})";
    EXPECT_THROW((FromStringAs<ValidatedMap, ValidateMapSize>(large_map_json)), parser::ParseError);

    EXPECT_EQ((FromStreamAs<ValidatedMap, ValidateMapSize>(good_json)), expected_map);
    EXPECT_THROW((FromStreamAs<ValidatedMap, ValidateMapSize>(bad_value_json)), parser::ParseError);
}

TEST(JsonAsTParser, UberComplexHierarchicalValidation) {
    using namespace formats::json;

    using ValidatedInt = parser::WithValidators<int, ValidatePositive>;
    using ValidatedSet = parser::WithValidators<std::set<ValidatedInt>, ValidateSetOfInts>;
    using ValidatedVectorOfSets = parser::WithValidators<std::vector<ValidatedSet>, ValidateVectorOfSets>;
    using ValidatedMap =
        parser::WithValidators<std::unordered_map<std::string, ValidatedVectorOfSets>, ValidateInnerMap>;
    using UberComplexType = std::vector<ValidatedMap>;

    const std::string good_json = R"([
        {
            "data_points": [
                [1, 2, 3],
                [100, 200]
            ],
            "metadata_points": [
                [5, 6]
            ]
        }
    ])";

    const std::vector<std::unordered_map<std::string, std::vector<std::set<int>>>> expected = {
        {{"data_points", {{1, 2, 3}, {100, 200}}}, {"metadata_points", {{5, 6}}}}
    };

    EXPECT_EQ((FromStringAs<UberComplexType, ValidateOuterVector>(good_json)), expected);

    const std::string bad_json_level_5 = R"([{"key": [[5, -6]]}])";
    EXPECT_THROW((FromStringAs<UberComplexType, ValidateOuterVector>(bad_json_level_5)), parser::ParseError);

    const std::string bad_json_level_4 = R"([{"key": [[]]}])";
    EXPECT_THROW((FromStringAs<UberComplexType, ValidateOuterVector>(bad_json_level_4)), parser::ParseError);

    const std::string bad_json_level_3 = R"([{"key": []}])";
    EXPECT_THROW((FromStringAs<UberComplexType, ValidateOuterVector>(bad_json_level_3)), parser::ParseError);

    const std::string bad_json_level_2 = R"([{}])";
    EXPECT_THROW((FromStringAs<UberComplexType, ValidateOuterVector>(bad_json_level_2)), parser::ParseError);

    const std::string bad_json_level_1 = R"([{"a":[[]]}, {"b":[[]]}, {"c":[[]]}])";
    EXPECT_THROW((FromStringAs<UberComplexType, ValidateOuterVector>(bad_json_level_1)), parser::ParseError);
}

struct MyObject {
    double k;
    double v;
    std::string s;

    static constexpr auto DescribeForJsonParsing() {
        return std::make_tuple(
            formats::json::parser::Field("k", &MyObject::k),
            formats::json::parser::Field("v", &MyObject::v),
            formats::json::parser::Field("s", &MyObject::s)
        );
    }

    bool operator==(const MyObject& other) const { return k == other.k && v == other.v && s == other.s; }
    bool operator!=(const MyObject& other) const { return !(*this == other); }
};

void ValidateMyObject(const MyObject& obj) {
    if (obj.k <= 0) {
        throw std::runtime_error("Field 'k' must be positive");
    }
    if (obj.s.empty()) {
        throw std::runtime_error("Field 's' must not be empty");
    }
}

TEST(JsonAsTParser, UniversalObjectParser_Simple) {
    using namespace formats::json;

    const std::string json_string = R"({"k": 123.45, "v": 1.11, "s": "hello world"})";
    const MyObject expected{123.45, 1.11, "hello world"};

    const auto result_from_string = FromStringAs<MyObject>(json_string);
    EXPECT_EQ(result_from_string, expected);

    const auto result_from_stream = FromStreamAs<MyObject>(json_string);
    EXPECT_EQ(result_from_stream, expected);

    const std::string shuffled_json = R"({"s": "shuffled", "k": 99.0, "v": -1.0})";
    const MyObject shuffled_expected{99.0, -1.0, "shuffled"};
    EXPECT_EQ(FromStringAs<MyObject>(shuffled_json), shuffled_expected);
}

TEST(JsonAsTParser, UniversalObjectParser_WithValidators) {
    using namespace formats::json;

    const std::string good_json = R"({"k": 10, "v": 5, "s": "valid"})";
    const MyObject expected{10, 5, "valid"};

    EXPECT_EQ((FromStringAs<MyObject, ValidateMyObject>(good_json)), expected);
    EXPECT_NO_THROW((FromStringAs<MyObject, ValidateMyObject>(good_json)));
    EXPECT_NO_THROW((FromStreamAs<MyObject, ValidateMyObject>(good_json)));

    const std::string bad_k_json = R"({"k": -1, "v": 5, "s": "valid"})";
    EXPECT_THROW((FromStringAs<MyObject, ValidateMyObject>(bad_k_json)), parser::ParseError);
    EXPECT_THROW((FromStreamAs<MyObject, ValidateMyObject>(bad_k_json)), parser::ParseError);

    const std::string bad_s_json = R"({"k": 10, "v": 5, "s": ""})";
    EXPECT_THROW((FromStringAs<MyObject, ValidateMyObject>(bad_s_json)), parser::ParseError);
    EXPECT_THROW((FromStreamAs<MyObject, ValidateMyObject>(bad_s_json)), parser::ParseError);
}

struct ComplexObject {
    int int_val;
    double double_val;
    std::string string_val;
    std::vector<int> vector_val;
    std::set<std::string> set_val;
    std::unordered_set<int> unordered_set_val;
    std::map<std::string, int> map_val;
    std::unordered_map<std::string, double> unordered_map_val;
    formats::json::Value raw_data;

    static constexpr auto DescribeForJsonParsing() {
        return std::make_tuple(
            formats::json::parser::Field("i", &ComplexObject::int_val),
            formats::json::parser::Field("d", &ComplexObject::double_val),
            formats::json::parser::Field("s", &ComplexObject::string_val),
            formats::json::parser::Field("vec", &ComplexObject::vector_val),
            formats::json::parser::Field("set", &ComplexObject::set_val),
            formats::json::parser::Field("uset", &ComplexObject::unordered_set_val),
            formats::json::parser::Field("map", &ComplexObject::map_val),
            formats::json::parser::Field("umap", &ComplexObject::unordered_map_val),
            formats::json::parser::Field("raw", &ComplexObject::raw_data)
        );
    }

    bool operator==(const ComplexObject& other) const {
        return int_val == other.int_val && double_val == other.double_val && string_val == other.string_val &&
               vector_val == other.vector_val && set_val == other.set_val &&
               unordered_set_val == other.unordered_set_val && map_val == other.map_val &&
               unordered_map_val == other.unordered_map_val;
    }
};

struct NestedObject {
    std::string id;
    int version;
    ComplexObject payload;
    formats::json::Value extra_info;

    static constexpr auto DescribeForJsonParsing() {
        return std::make_tuple(
            formats::json::parser::Field("id", &NestedObject::id),
            formats::json::parser::Field("version", &NestedObject::version),
            formats::json::parser::Field("payload", &NestedObject::payload),
            formats::json::parser::Field("extra", &NestedObject::extra_info)
        );
    }

    bool operator==(const NestedObject& other) const {
        return id == other.id && version == other.version && payload == other.payload;
    }
};
TEST(JsonAsTParser, UniversalObjectParser_ComplexFields) {
    using namespace formats::json;

    const std::string json_string = R"({
        "i": -100, "d": 99.99, "s": "complex object",
        "vec": [1, 2, 3],
        "set": ["alpha", "beta", "gamma", "alpha"],
        "uset": [10, 20, 30, 10],
        "map": {"c": 3, "a": 1, "b": 2},
        "umap": {"pi": 3.14, "e": 2.71},
        "raw": {"a": [1, null], "b": "any"}
    })";

    formats::json::ValueBuilder raw_builder;
    raw_builder["a"].PushBack(1);
    raw_builder["a"].PushBack(nullptr);
    raw_builder["b"] = "any";

    const ComplexObject expected{
        -100,
        99.99,
        "complex object",
        {1, 2, 3},
        {"alpha", "beta", "gamma"},
        {10, 20, 30},
        {{"a", 1}, {"b", 2}, {"c", 3}},
        {{"pi", 3.14}, {"e", 2.71}},
        raw_builder.ExtractValue()
    };

    const auto result = FromStringAs<ComplexObject>(json_string);
    EXPECT_EQ(result, expected);

    std::unordered_map<std::string, formats::json::Value> expected_raw_data;
    expected_raw_data["a"] = formats::json::FromString("[1, null]");
    formats::json::ValueBuilder string_builder;
    string_builder = "any";
    expected_raw_data["b"] = string_builder.ExtractValue();

    EXPECT_EQ((result.raw_data.As<std::unordered_map<std::string, formats::json::Value>>()), expected_raw_data);
}

TEST(JsonAsTParser, UniversalObjectParser_NestedObjects) {
    using namespace formats::json;

    const std::string complex_part = R"({
        "i": -100, "d": 99.99, "s": "complex object",
        "vec": [1, 2, 3], "set": ["alpha", "beta"], "uset": [10, 20],
        "map": {"a": 1}, "umap": {"pi": 3.14},
        "raw": true
    })";

    const std::string nested_json_string = fmt::format(
        R"({{
        "id": "request-123",
        "version": 2,
        "payload": {},
        "extra": [ "some", "extra", "data" ]
    }})",
        complex_part
    );

    formats::json::ValueBuilder raw_payload_builder;
    raw_payload_builder = true;
    formats::json::Value raw_payload_value = raw_payload_builder.ExtractValue();

    formats::json::Value extra_info_value = formats::json::FromString(R"([ "some", "extra", "data" ])");

    const NestedObject expected{
        "request-123",
        2,
        {-100,
         99.99,
         "complex object",
         {1, 2, 3},
         {"alpha", "beta"},
         {10, 20},
         {{"a", 1}},
         {{"pi", 3.14}},
         std::move(raw_payload_value)},
        std::move(extra_info_value)
    };

    const auto result = FromStringAs<NestedObject>(nested_json_string);
    EXPECT_EQ(result, expected);

    const std::vector<std::string> expected_extra = {"some", "extra", "data"};
    EXPECT_EQ(result.extra_info.As<std::vector<std::string>>(), expected_extra);
}

USERVER_NAMESPACE_END
