#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/variant.hpp>

#include <userver/chaotic/exception.hpp>

#include <schemas/date.hpp>
#include <schemas/date_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, Date) {
    auto json = formats::json::MakeObject("created_at", "2020-10-01");
    auto obj = json.As<ns::ObjectDate>();
    EXPECT_EQ(obj.created_at, utils::datetime::Date(2020, 10, 01));
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, DateSax) {
    auto json = formats::json::MakeObject("created_at", "2020-10-01");
    auto obj = CallSaxParser<ns::ObjectDate>(ToString(json));
    EXPECT_EQ(obj.created_at, utils::datetime::Date(2020, 10, 01));
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Simple, DateTime) {
    auto date = "2020-10-01T12:34:56+12:34";
    auto json = formats::json::MakeObject("updated_at", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTz
        tp{utils::datetime::UtcStringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.updated_at, tp);
    EXPECT_EQ(TestToJsonString(obj), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["updated_at"].As<std::string>();
    EXPECT_EQ(str, date);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, DateTimeExtra) {
    auto date = "2020-10-01T12:34:56+12:34";
    auto json = formats::json::MakeObject("updated_at_extra", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTz
        tp{utils::datetime::UtcStringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.updated_at_extra->time_point, tp);
    EXPECT_EQ(TestToJsonString(obj), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["updated_at_extra"].As<std::string>();
    EXPECT_EQ(str, date);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, DateTimeIsoBasic) {
    auto date = "2020-10-01T12:34:56+1234";
    auto json = formats::json::MakeObject("deleted_at", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTzIsoBasic
        tp{utils::datetime::UtcStringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.deleted_at, tp);
    EXPECT_EQ(TestToJsonString(obj), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["deleted_at"].As<std::string>();
    EXPECT_EQ(str, date);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, DateTimeFraction) {
    auto date = "2020-10-01T12:34:56.789+0000";
    auto json = formats::json::MakeObject("modified_at", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTz
        tp{utils::datetime::UtcStringtime("2020-10-01T12:34:56Z"), std::chrono::seconds(0)};
    EXPECT_EQ(obj.modified_at->GetTimePoint() - tp.GetTimePoint(), std::chrono::milliseconds(789));
    EXPECT_EQ(obj.modified_at->GetTzOffset(), std::chrono::seconds(0));
    EXPECT_EQ(TestToJsonString(obj), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["modified_at"].As<std::string>();
    EXPECT_EQ(str, date) << str;

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, DateInvalid) {
    auto json = formats::json::MakeObject("created_at", "not-a-date");
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectDate>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'created_at': Can't parse datetime: not-a-date"
    );
}

TEST(Simple, DateTimeInvalid) {
    auto json = formats::json::MakeObject("updated_at", "not-a-datetime");
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectDate>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'updated_at': Can't parse datetime: not-a-datetime"
    );
}

USERVER_NAMESPACE_END
