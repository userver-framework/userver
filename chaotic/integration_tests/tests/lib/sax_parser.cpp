#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/sax_parser.hpp>
#include <userver/chaotic/validators_pattern.hpp>
#include <userver/formats/json/inline.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace sax = chaotic::sax;

TEST(SaxParser, Bool)
{
    using Parser = sax::Parser<chaotic::Primitive<bool>>;
    auto input = "true";
    auto result = formats::json::parser::ParseToType<bool, Parser>(input);
    EXPECT_TRUE(result);
}

TEST(SaxParser, Int32)
{
    using Parser = sax::Parser<chaotic::Primitive<std::int32_t>>;
    auto input = "123";
    auto result = formats::json::parser::ParseToType<std::int32_t, Parser>(input);
    EXPECT_EQ(result, 123);
}

TEST(SaxParser, Int64)
{
    using Parser = sax::Parser<chaotic::Primitive<std::int64_t>>;
    auto input = "1234";
    auto result = formats::json::parser::ParseToType<std::int64_t, Parser>(input);
    EXPECT_EQ(result, 1234);
}

TEST(SaxParser, Double)
{
    using Parser = sax::Parser<chaotic::Primitive<double>>;
    auto input = "12.34";
    auto result = formats::json::parser::ParseToType<double, Parser>(input);
    EXPECT_DOUBLE_EQ(result, 12.34);
}

TEST(SaxParser, Float)
{
    using Parser = sax::Parser<chaotic::Primitive<float>>;
    auto input = "12.34";
    auto result = formats::json::parser::ParseToType<float, Parser>(input);
    EXPECT_FLOAT_EQ(result, 12.34);
}

TEST(SaxParser, String)
{
    using Parser = sax::Parser<chaotic::Primitive<std::string>>;
    auto input = "\"foo bar baz\"";
    auto result = formats::json::parser::ParseToType<std::string, Parser>(input);
    EXPECT_EQ(result, "foo bar baz");
}

TEST(SaxParser, StringPattern) {
    static constexpr std::string_view kPatternFooStar = "foo.*";

    using Parser = sax::Parser<chaotic::Primitive<std::string, chaotic::Pattern<kPatternFooStar>>>;
    auto input = "\"not matching\"";

    UEXPECT_THROW_MSG(
        (formats::json::parser::ParseToType<std::string, Parser>(input)),
        formats::json::parser::ParseError,
        "doesn't match regex"
    );
}

TEST(SaxParser, PrimitiveValidator)
{
    static constexpr auto kMinimum = 10;
    using Parser = sax::Parser<chaotic::Primitive<std::int32_t, chaotic::Minimum<kMinimum>>>;

    auto input = "123";
    auto result = formats::json::parser::ParseToType<std::int32_t, Parser>(input);
    EXPECT_EQ(result, 123);

    input = "1";
    UEXPECT_THROW((formats::json::parser::ParseToType<std::int32_t, Parser>(input)), formats::json::parser::ParseError);
}

enum class EnumInt {
    kOne,
    kTwo,
};

EnumInt Convert(std::int64_t i, chaotic::convert::To<EnumInt>) {
    switch (i) {
        case 1:
            return EnumInt::kOne;
        case 2:
            return EnumInt::kTwo;
        default:
            UINVARIANT(false, "");
    }
}

TEST(SaxParser, EnumInt)
{
    using Parser = sax::Parser<EnumInt>;

    auto input = "1";
    auto result = formats::json::parser::ParseToType<EnumInt, Parser>(input);
    EXPECT_EQ(result, EnumInt::kOne);
}

enum class EnumString {
    kOne,
    kTwo,
};

EnumString Convert(std::string_view str, chaotic::convert::To<EnumString>) {
    if (str == "one") {
        return EnumString::kOne;
    } else if (str == "two") {
        return EnumString::kTwo;
    } else {
        UINVARIANT(false, "");
    }
}

TEST(SaxParser, EnumString)
{
    using Parser = sax::Parser<EnumString>;

    auto input = "\"one\"";
    auto result = formats::json::parser::ParseToType<EnumString, Parser>(input);
    EXPECT_EQ(result, EnumString::kOne);
}

TEST(SaxParser, Array)
{
    using Parser = sax::Parser<chaotic::Array<std::int32_t, std::vector<std::int32_t>>>;

    auto input = "[1, 2, 3]";
    auto result = formats::json::parser::ParseToType<std::vector<std::int32_t>, Parser>(input);
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(SaxParser, ArrayRaw)
{
    using Parser = sax::Parser<std::vector<std::int32_t>>;

    auto input = "[1, 2, 3]";
    auto result = formats::json::parser::ParseToType<std::vector<std::int32_t>, Parser>(input);
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(SaxParser, ArrayWithValidator)
{
    using Parser = sax::Parser<chaotic::Array<std::int32_t, std::vector<std::int32_t>, chaotic::MinItems<1>>>;

    auto input = "[1, 2, 3]";
    auto result = formats::json::parser::ParseToType<std::vector<std::int32_t>, Parser>(input);
    EXPECT_EQ(result, (std::vector{1, 2, 3}));

    input = "[]";
    UEXPECT_THROW(
        (formats::json::parser::ParseToType<std::vector<std::int32_t>, Parser>(input)),
        formats::json::parser::ParseError
    );
}

TEST(SaxParser, ArrayOfArrayOfInt)
{
    using ArrayOfArray =
        chaotic::Array<chaotic::Array<std::int32_t, std::vector<std::int32_t>>, std::vector<std::vector<std::int32_t>>>;
    using Parser = sax::Parser<ArrayOfArray>;

    auto input = "[[1, 2, 3]]";
    auto result = formats::json::parser::ParseToType<std::vector<std::vector<int>>, Parser>(input);
    EXPECT_EQ(result, (std::vector<std::vector<int>>{std::vector{1, 2, 3}}));
}

struct X {
    int field{99};
    std::optional<bool> ok;
    std::string data;

    formats::json::Value extra;

    auto tie() const { return std::tie(field, ok, data, extra); }

    bool operator==(const X& other) const { return tie() == other.tie(); }
};

constexpr utils::StringLiteral kField = "field";
constexpr utils::StringLiteral kData = "data";
constexpr utils::StringLiteral kOk = "ok";

constexpr std::string_view kDataDefault = "foo";

TEST(SaxParser, ObjectUnknownIgnore)
{
    using Parser = sax::Parser<chaotic::Object<
        X,
        chaotic::UnknownFields::Ignore,
        chaotic::Field<X, chaotic::Required<int>, &X::field, kField>,
        chaotic::Field<X, chaotic::Defaulted<std::string, std::string_view, kDataDefault>, &X::data, kData>,
        chaotic::Field<X, chaotic::Optional<bool>, &X::ok, kOk>>>;

    auto input = "{\"field\": 12}";
    auto result = formats::json::parser::ParseToType<X, Parser>(input);
    EXPECT_EQ(result, (X{12, {}, "foo", {}}));

    input = R"({"field": 12, "unknown": 55})";
    result = formats::json::parser::ParseToType<X, Parser>(input);
    EXPECT_EQ(result, (X{12, {}, "foo", {}}));

    input = R"({"field": 13, "ok": true})";
    result = formats::json::parser::ParseToType<X, Parser>(input);
    EXPECT_EQ(result, (X{13, true, "foo", {}}));

    input = "{}";
    UEXPECT_THROW((formats::json::parser::ParseToType<X, Parser>(input)), formats::json::parser::ParseError);
}

TEST(SaxParser, ObjectUnknownStore)
{
    using Parser = sax::Parser<chaotic::Object<
        X,
        chaotic::UnknownFields::StoreJson,
        chaotic::Field<X, chaotic::Required<int>, &X::field, kField>,
        chaotic::Field<X, chaotic::Defaulted<std::string, std::string_view, kDataDefault>, &X::data, kData>,
        chaotic::Field<X, chaotic::Optional<bool>, &X::ok, kOk>>>;

    auto input = R"({"field": 12, "foo": 42, "bar": false})";
    auto result = formats::json::parser::ParseToType<X, Parser>(input);
    EXPECT_EQ(result, (X{12, {}, "foo", formats::json::MakeObject("foo", 42, "bar", false)}));
}

TEST(SaxParser, ObjectUnknownError)
{
    using Parser = sax::Parser<chaotic::Object<
        X,
        chaotic::UnknownFields::Forbid,
        chaotic::Field<X, chaotic::Required<int>, &X::field, kField>,
        chaotic::Field<X, chaotic::Defaulted<std::string, std::string_view, kDataDefault>, &X::data, kData>,
        chaotic::Field<X, chaotic::Optional<bool>, &X::ok, kOk>>>;

    auto input = R"({"field": 12, "unknown": 42})";
    UEXPECT_THROW((formats::json::parser::ParseToType<X, Parser>(input)), formats::json::parser::ParseError);
}

}  // namespace

struct SelfRef {
    // int data{1};
    std::optional<utils::Box<SelfRef>> next;
};

template <typename Value>
SelfRef Parse(const Value& /*value*/, formats::parse::To<SelfRef>)
{
    // TODO
    return {};
}

constexpr utils::StringLiteral kNext = "next";

using SelfRefDescriptor = chaotic::Object<
    SelfRef,
    chaotic::UnknownFields::Forbid,
    chaotic::Field<SelfRef, chaotic::Optional<chaotic::Ref<SelfRef>>, &SelfRef::next, kNext>>;

using SelfRefParser = sax::Parser<SelfRefDescriptor>;

[[maybe_unused]] auto ParserOf(chaotic::sax::Type<SelfRef>) -> SelfRefParser;

TEST(SaxParser, ObjectSelfRef)
{
    using Parser = SelfRefParser;

    auto input = R"({"field": 12, "unknown": 42})";
    UEXPECT_THROW((formats::json::parser::ParseToType<SelfRef, Parser>(input)), formats::json::parser::ParseError);
}

USERVER_NAMESPACE_END
