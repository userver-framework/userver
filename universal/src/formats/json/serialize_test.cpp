#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <map>
#include <ranges>

#include <formats/common/serialize_test.hpp>
#include <userver/formats/json/exception.hpp>
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
        EXPECT_THAT(std::string_view{e.what()}, testing::HasSubstr(expected_msg))
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
        EXPECT_THAT(std::string_view{e.what()}, testing::HasSubstr(filename))
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
    const std::string& str = GetParam();
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
    const NotSortedTestData& pair_data_res = GetParam();
    const auto json = formats::json::FromString(pair_data_res.source);
    EXPECT_EQ(formats::json::ToStableString(json), pair_data_res.result);
}

TEST_P(NonSortedJsonToSortedString, NonDepthTreeMove) {
    const NotSortedTestData& pair_data_res = GetParam();
    auto json = formats::json::FromString(pair_data_res.source);
    EXPECT_EQ(formats::json::ToStableString(std::move(json)), pair_data_res.result);
}

TEST_P(JsonToStringCycle, NonDepthTree) {
    const CycleTestData& data = GetParam();
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

TEST(JsonToSortedString, DoubleFormatting) {
    const auto value = formats::json::FromString("1e-5");
    EXPECT_EQ(formats::json::ToStableString(value), "0.00001");
}

TEST(JsonToSortedString, StringEscaping) {
    const auto value = formats::json::ValueBuilder(std::string{"\"/\\\v\x1e"}).ExtractValue();
    EXPECT_EQ(formats::json::ToStableString(value), R"("\"/\\\u000B\u001E")");
}

TEST(JsonToPythonCompatibleStableString, ConstAndRvalueOverloads) {
    const auto value = formats::json::FromString(R"({"b":[3,{"b":2,"a":1}],"a":0})");
    EXPECT_EQ(formats::json::ToPythonCompatibleStableString(value), R"({"a":0,"b":[3,{"a":1,"b":2}]})");

    auto movable_value = value.Clone();
    EXPECT_EQ(
        formats::json::ToPythonCompatibleStableString(std::move(movable_value)),
        R"({"a":0,"b":[3,{"a":1,"b":2}]})"
    );
}

TEST(JsonToPythonCompatibleStableString, SortsByUnicodeCodePointsRecursively) {
    const auto value = formats::json::FromString(R"({"😀":{"😀":2,"":1},"":0})");
    EXPECT_EQ(formats::json::ToPythonCompatibleStableString(value), R"({"":0,"😀":{"":1,"😀":2}})");
}

TEST(JsonToPythonCompatibleStableString, PythonStringEscaping) {
    std::string source{"\"/\\\b\t\n\f\r"};
    source.append("\0\1\v\x1f", 4);
    source += "\x7f元";

    const auto value = formats::json::ValueBuilder(source).ExtractValue();
    EXPECT_EQ(
        formats::json::ToPythonCompatibleStableString(value),
        "\"\\\"/\\\\\\b\\t\\n\\f\\r\\u0000\\u0001\\u000b\\u001f\x7f元\""
    );
}

TEST(JsonToPythonCompatibleStableString, AcceptsValidUtf8BoundaryCodePoints) {
    const std::string source{
        "\xc2\x80"          // U+0080, first two-byte scalar value
        "\xdf\xbf"          // U+07FF, last two-byte scalar value
        "\xe0\xa0\x80"      // U+0800, first three-byte scalar value
        "\xed\x9f\xbf"      // U+D7FF, immediately before surrogates
        "\xee\x80\x80"      // U+E000, immediately after surrogates
        "\xef\xbf\xbf"      // U+FFFF, valid UTF-8 noncharacter
        "\xf0\x90\x80\x80"  // U+10000, first four-byte scalar value
        "\xf4\x8f\xbf\xbf"  // U+10FFFF, last Unicode scalar value
    };
    const auto value = formats::json::ValueBuilder(source).ExtractValue();
    EXPECT_EQ(formats::json::ToPythonCompatibleStableString(value), '"' + source + '"');
}

TEST(JsonToPythonCompatibleStableString, DoesNotNormalizeUnicode) {
    const auto value = formats::json::FromString(R"(["é","é"])");
    EXPECT_EQ(formats::json::ToPythonCompatibleStableString(value), R"(["é","é"])");
}

TEST(JsonToPythonCompatibleStableString, ExactIntegers) {
    EXPECT_EQ(
        formats::json::ToPythonCompatibleStableString(
            formats::json::ValueBuilder(std::numeric_limits<std::int64_t>::min()).ExtractValue()
        ),
        "-9223372036854775808"
    );
    EXPECT_EQ(
        formats::json::ToPythonCompatibleStableString(
            formats::json::ValueBuilder(std::numeric_limits<std::uint64_t>::max()).ExtractValue()
        ),
        "18446744073709551615"
    );
}

TEST(JsonToPythonCompatibleStableString, PythonDoubleFormatting) {
    struct TestCase final {
        std::uint64_t bits;
        std::string_view expected;
    };
    static constexpr std::array kTestCases{
        TestCase{0x0000000000000000, "0.0"},
        TestCase{0x8000000000000000, "-0.0"},
        TestCase{0x3ff0000000000000, "1.0"},
        TestCase{0x3ff8000000000000, "1.5"},
        TestCase{0x3f1a36e2eb1c432d, "0.0001"},
        TestCase{0x3ee4f8b588e368f1, "1e-05"},
        TestCase{0x430c6bf526340000, "1000000000000000.0"},
        TestCase{0x4341c37937e08000, "1e+16"},
        TestCase{0x0000000000000001, "5e-324"},
        TestCase{0x0010000000000000, "2.2250738585072014e-308"},
        TestCase{0x7fefffffffffffff, "1.7976931348623157e+308"},
        TestCase{0x41b3de4355555555, "333333333.3333333"},
        TestCase{0x3ff3c0ca428c59fb, "1.2345678901234567"},
        TestCase{0x3eb0c6f7a0b5ed8d, "1e-06"},
        TestCase{0x3eb0c6f7a0b5ed8c, "9.999999999999997e-07"},
        TestCase{0x3f1a36e2eb1c432c, "9.999999999999999e-05"},
        TestCase{0x3f1a36e2eb1c432e, "0.00010000000000000002"},
        TestCase{0x4341c37937e07fff, "9999999999999998.0"},
        TestCase{0x4341c37937e08001, "1.0000000000000002e+16"},
        TestCase{0xbf1a36e2eb1c432d, "-0.0001"},
        TestCase{0xbee4f8b588e368f1, "-1e-05"},
        TestCase{0xc30c6bf526340000, "-1000000000000000.0"},
        TestCase{0xc341c37937e08000, "-1e+16"},
        TestCase{0x0010000000000001, "2.225073858507202e-308"},
        TestCase{0x000fffffffffffff, "2.225073858507201e-308"},
        TestCase{0x7feffffffffffffe, "1.7976931348623155e+308"},
        TestCase{0x0123456789abcdef, "3.512700564088504e-303"},
        TestCase{0x400921fb54442d18, "3.141592653589793"},
        TestCase{0x2b2bff2ee48e0530, "1e-100"},
        TestCase{0x59a3d89f129165e9, "6.55971696577832e+123"},
        TestCase{0x0941858a7c65480a, "4.347190509248525e-264"},
        TestCase{0xc319c2427df2aecd, "-1812616313613235.2"},
        TestCase{0xc2b517c30a8f1250, "-23191800680210.312"},
        TestCase{0x4319c2427df2aecd, "1812616313613235.2"},
    };

    for (const auto& test : kTestCases) {
        const auto value = formats::json::ValueBuilder(std::bit_cast<double>(test.bits)).ExtractValue();
        EXPECT_EQ(formats::json::ToPythonCompatibleStableString(value), test.expected)
            << fmt::format("bits={:#018x}", test.bits);
    }
}

TEST(JsonToPythonCompatibleStableString, RejectsInvalidUtf8) {
    const std::array<std::string_view, 9> invalid_values{
        std::string_view{"\x80", 1},
        std::string_view{"\xff", 1},
        std::string_view{"\xc0\xaf", 2},
        std::string_view{"\xe0\x80\x80", 3},
        std::string_view{"\xe2\x28\xa1", 3},
        std::string_view{"\xed\xa0\x80", 3},
        std::string_view{"\xf0\x9f\x98", 3},
        std::string_view{"\xf0\x28\x8c\xbc", 4},
        std::string_view{"\xf4\x90\x80\x80", 4},
    };
    for (const auto invalid : invalid_values) {
        const auto value = formats::json::ValueBuilder(invalid).ExtractValue();
        EXPECT_THROW(formats::json::ToPythonCompatibleStableString(value), formats::json::Exception);
    }

    formats::json::ValueBuilder object(formats::common::Type::kObject);
    object.EmplaceNocheck(std::string_view{"\xed\xa0\x80", 3}, formats::json::ValueBuilder{1});
    EXPECT_THROW(formats::json::ToPythonCompatibleStableString(object.ExtractValue()), formats::json::Exception);
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

USERVER_NAMESPACE_END
