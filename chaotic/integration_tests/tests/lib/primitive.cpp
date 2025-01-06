#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/validators_pattern.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const auto kJson = formats::json::MakeObject("foo", 1, "bar", 0, "zoo", 6);
}

TEST(Primitive, NoValidator) {
    using Int = chaotic::Primitive<std::int32_t>;
    int x = kJson["foo"].As<Int>();
    EXPECT_EQ(x, 1);
}

TEST(Primitive, NoValidatorSerializer) {
    using Int = chaotic::Primitive<std::int32_t>;
    EXPECT_EQ(formats::json::ValueBuilder{Int{kJson["foo"].As<Int>()}}.ExtractValue(), kJson["foo"]);
}

struct MyInt {
    MyInt(std::int32_t value) : value(value) {}

    std::int32_t value;
};

MyInt Convert(const std::int32_t& i, chaotic::convert::To<MyInt>) { return {i}; }

std::int32_t Convert(const MyInt& i, chaotic::convert::To<std::int32_t>) { return i.value; }

TEST(Primitive, UserType) {
    using Int = chaotic::WithType<chaotic::Primitive<std::int32_t>, MyInt>;
    MyInt x = kJson["foo"].As<Int>();
    EXPECT_EQ(x.value, 1);
}

TEST(Primitive, UserTypeSerializer) {
    using Int = chaotic::WithType<chaotic::Primitive<std::int32_t>, MyInt>;
    MyInt x = kJson["foo"].As<Int>();

    EXPECT_EQ(formats::json::ValueBuilder{Int{x}}.ExtractValue(), kJson["foo"]);
}

TEST(Primitive, WrongType) {
    using String = chaotic::Primitive<std::string>;
    try {
        std::string x = kJson["foo"].As<String>();
    } catch (const std::exception& e) {
        EXPECT_EQ(
            std::string(e.what()),
            "Error at path 'foo': Wrong type. Expected: stringValue, actual: "
            "intValue"
        );
    }
}

constexpr auto kOne = 1;
constexpr auto kFive = 5;

TEST(Primitive, IntMinMax) {
    using Int = chaotic::Primitive<std::int32_t, chaotic::Minimum<kOne>, chaotic::Maximum<kFive>>;

    int x = kJson["foo"].As<Int>();
    EXPECT_EQ(x, 1);

    UEXPECT_THROW_MSG(kJson["bar"].As<Int>(), chaotic::Error, "Error at path 'bar': Invalid value, minimum=1, given=0");
    UEXPECT_THROW_MSG(kJson["zoo"].As<Int>(), chaotic::Error, "Error at path 'zoo': Invalid value, maximum=5, given=6");
}

TEST(Primitive, UserTypeMinMax) {
    using Int =
        chaotic::WithType<chaotic::Primitive<std::int32_t, chaotic::Minimum<kOne>, chaotic::Maximum<kFive>>, MyInt>;

    MyInt x = kJson["foo"].As<Int>();
    EXPECT_EQ(x.value, 1);

    UEXPECT_THROW_MSG(kJson["bar"].As<Int>(), chaotic::Error, "Error at path 'bar': Invalid value, minimum=1, given=0");
    UEXPECT_THROW_MSG(kJson["zoo"].As<Int>(), chaotic::Error, "Error at path 'zoo': Invalid value, maximum=5, given=6");
}

TEST(Primitive, StringMinMaxLength) {
    auto kLocalJson = formats::json::MakeObject("1", "1", "2", "12", "6", "123456");

    using Str = chaotic::Primitive<std::string, chaotic::MinLength<2>, chaotic::MaxLength<5>>;

    std::string x = kLocalJson["2"].As<Str>();
    EXPECT_EQ(x, "12");

    UEXPECT_THROW_MSG(
        kLocalJson["1"].As<Str>(), chaotic::Error, "Error at path '1': Too short string, minimum length=2, given=1"
    );
    UEXPECT_THROW_MSG(
        kLocalJson["6"].As<Str>(), chaotic::Error, "Error at path '6': Too long string, maximum length=5, given=6"
    );
}

static constexpr std::string_view kPattern = "fo.*";

TEST(Primitive, StringPattern) {
    auto kLocalJson = formats::json::MakeObject("1", "foo", "2", "bar");

    using Str = chaotic::Primitive<std::string, chaotic::Pattern<kPattern>>;

    std::string x = kLocalJson["1"].As<Str>();
    EXPECT_EQ(x, "foo");

    UEXPECT_THROW_MSG(kLocalJson["2"].As<Str>(), chaotic::Error, "doesn't match regex");
}

USERVER_NAMESPACE_END
