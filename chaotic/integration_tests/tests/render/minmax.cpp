#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/parser/exception.hpp>

#include <schemas/int_minmax.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MinMax, ExclusiveInt) {
    auto json = formats::json::MakeObject("foo", 1);
    UEXPECT_THROW_MSG(
        json.As<ns::IntegerObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'foo': Invalid value, exclusive minimum=1, given=1"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 8, path 'foo': Error at path 'foo': Invalid value, exclusive minimum=1, given=1, the "
        "latest token was :1"
    );

    json = formats::json::MakeObject("foo", 2);
    EXPECT_EQ(json.As<ns::IntegerObject>().foo, 2);
    EXPECT_EQ(FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()).foo, 2);

    json = formats::json::MakeObject("foo", 20);
    UEXPECT_THROW_MSG(
        json.As<ns::IntegerObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'foo': Invalid value, exclusive maximum=20, given=20"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 9, path 'foo': Error at path 'foo': Invalid value, exclusive maximum=20, given=20, the "
        "latest token was :20"
    );

    json = formats::json::MakeObject("foo", 19);
    EXPECT_EQ(json.As<ns::IntegerObject>().foo, 19);
}

TEST(MinMax, String) {
    auto json = formats::json::MakeObject("bar", "");
    UEXPECT_THROW_MSG(
        json.As<ns::IntegerObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'bar': Too short string, minimum length=2, given=0"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 9, path 'bar': Error at path 'bar': Too short string, minimum length=2, given=0, the "
        "latest token was :\"\""
    );

    json = formats::json::MakeObject("bar", "longlonglong");
    UEXPECT_THROW_MSG(
        json.As<ns::IntegerObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'bar': Too long string, maximum length=5, given=12"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 21, path 'bar': Error at path 'bar': Too long string, maximum length=5, given=12, the "
        "latest token was :\"longlonglong\""
    );
}

TEST(MinMax, Array) {
    auto json = formats::json::MakeObject("zoo", formats::json::MakeArray(1));
    UEXPECT_THROW_MSG(
        json.As<ns::IntegerObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'zoo': Too short array, minimum length=2, given=1"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 9, path 'zoo': Error at path 'zoo': Too short array, minimum length=2, given=1"
    );

    json = formats::json::MakeObject("zoo", formats::json::MakeArray(1, 2, 3, 4, 5, 6, 7, 8));
    UEXPECT_THROW_MSG(
        json.As<ns::IntegerObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'zoo': Too long array, maximum length=5, given=8"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::IntegerObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 23, path 'zoo': Error at path 'zoo': Too long array, maximum length=5, given=8"
    );
}

USERVER_NAMESPACE_END
