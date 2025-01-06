#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <boost/uuid/string_generator.hpp>

#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/variant.hpp>

#include <schemas/all_of.hpp>
#include <schemas/date.hpp>
#include <schemas/extra_container.hpp>
#include <schemas/indirect.hpp>
#include <schemas/object_empty.hpp>
#include <schemas/object_name.hpp>
#include <schemas/object_object.hpp>
#include <schemas/object_single_field.hpp>
#include <schemas/one_of.hpp>
#include <schemas/uuid.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Simple, Empty) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::ObjectEmpty>();
    EXPECT_EQ(obj, ns::ObjectEmpty());
}

TEST(Simple, Integer) {
    auto json = formats::json::MakeObject("int3", 1, "integer", 3);
    auto obj = json.As<ns::SimpleObject>();
    EXPECT_EQ(obj.integer, 3);
}

TEST(Simple, DefaultFieldValue) {
    ns::SimpleObject obj;
    EXPECT_EQ(obj.int_, 1);
}

TEST(Simple, IntegerMinimum) {
    auto json = formats::json::MakeObject("int3", 1, "int", -10);
    UEXPECT_THROW_MSG(
        json.As<ns::SimpleObject>(), chaotic::Error, "Error at path 'int': Invalid value, minimum=-1, given=-10"
    );
}

TEST(Simple, IntegerMaximum) {
    auto json = formats::json::MakeObject("int3", 1, "int", 11);
    UEXPECT_THROW_MSG(
        json.As<ns::SimpleObject>(), chaotic::Error, "Error at path 'int': Invalid value, maximum=10, given=11"
    );
}

TEST(Simple, ObjectDefault) {
    auto json = formats::json::MakeObject("int3", 1);
    auto obj = json.As<ns::SimpleObject>();
    EXPECT_EQ(obj.int_, 1);
}

TEST(Simple, ObjectRequired) {
    auto json = formats::json::MakeObject();
    UEXPECT_THROW_MSG(
        json.As<ns::SimpleObject>(), formats::json::MemberMissingException, "Error at path 'int3': Field is missing"
    );
}

TEST(Simple, IntegerFormat) {
    static_assert(std::is_same_v<decltype(ns::SimpleObject::integer), std::optional<int>>);
    static_assert(std::is_same_v<decltype(ns::SimpleObject::int_), std::int32_t>);
    static_assert(std::is_same_v<decltype(ns::SimpleObject::int3), std::int64_t>);
}

TEST(Simple, ObjectWithRefType) {
    auto json = formats::json::MakeObject("integer", 0);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectWithRef>(), chaotic::Error, "Error at path 'integer': Invalid value, minimum=1, given=0"
    );
}

TEST(Simple, ObjectTypes) {
    auto json = formats::json::MakeObject(
        "integer",
        1,
        "boolean",
        true,
        "number",
        1.1,
        "string",
        "foo",
        "string-enum",
        "1",
        "object",
        formats::json::MakeObject(),
        "array",
        formats::json::MakeArray(1)
    );
    auto obj = json.As<ns::ObjectTypes>();
    EXPECT_EQ(
        obj,
        (ns::ObjectTypes{true, 1, 1.1, "foo", ns::ObjectTypes::Object{}, {1}, {}, ns::ObjectTypes::String_Enum::kX1})
    );
}

TEST(Simple, ObjectWithAdditionalPropertiesInt) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3);
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesInt>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, (std::unordered_map<std::string, int>{{"two", 2}, {"three", 3}}));

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, ObjectWithAdditionalPropertiesTrue) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesTrue>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, formats::json::MakeObject("two", 2, "three", 3, "object", formats::json::MakeObject()));

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, ObjectExtraMemberFalse) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesTrueExtraMemberFalse>();

    EXPECT_EQ(obj.one, 1);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, formats::json::MakeObject("one", 1)) << ToString(json_back);
}

TEST(Simple, ObjectWithAdditionalPropertiesFalseStrict) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectWithAdditionalPropertiesFalseStrict>(), std::runtime_error, "Unknown property 'bar'"
    );
}

TEST(Simple, IntegerEnum) {
    auto json = formats::json::MakeObject("one", 1);
    auto obj = json["one"].As<ns::IntegerEnum>();
    EXPECT_EQ(obj, ns::IntegerEnum::k1);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json["one"]) << ToString(json_back);

    auto json2 = formats::json::MakeObject("one", 5);
    UEXPECT_THROW_MSG(
        json2["one"].As<ns::IntegerEnum>(),
        chaotic::Error,
        "Error at path 'one': Invalid enum value (5) for type ns::IntegerEnum"
    );

    EXPECT_EQ(std::size(ns::kIntegerEnumValues), 3);

    ns::IntegerEnum values[] = {ns::IntegerEnum::k1, ns::IntegerEnum::k2, ns::IntegerEnum::k3};
    std::size_t index = 0;
    for (const auto& value : ns::kIntegerEnumValues) {
        EXPECT_EQ(value, values[index]);
        ++index;
    }
}

TEST(Simple, StringEnum) {
    auto json = formats::json::MakeObject("one", "foo");
    auto obj = json["one"].As<ns::StringEnum>();
    EXPECT_EQ(obj, ns::StringEnum::kFoo);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json["one"]) << ToString(json_back);

    auto json2 = formats::json::MakeObject("one", "zoo");
    UEXPECT_THROW_MSG(
        json2["one"].As<ns::StringEnum>(),
        chaotic::Error,
        "Error at path 'one': Invalid enum value (zoo) for type ns::StringEnum"
    );

    EXPECT_EQ("foo", ToString(ns::StringEnum::kFoo));
    EXPECT_EQ("bar", ToString(ns::StringEnum::kBar));
    EXPECT_EQ("some!thing", ToString(ns::StringEnum::kSomeThing));

    EXPECT_EQ(FromString("foo", formats::parse::To<ns::StringEnum>{}), ns::StringEnum::kFoo);
    UEXPECT_THROW_MSG(
        FromString("zoo", formats::parse::To<ns::StringEnum>{}),
        std::runtime_error,
        "Invalid enum value (zoo) for type ns::StringEnum"
    );

    EXPECT_EQ(Parse("foo", formats::parse::To<ns::StringEnum>{}), ns::StringEnum::kFoo);
    UEXPECT_THROW_MSG(
        Parse("zoo", formats::parse::To<ns::StringEnum>{}),
        std::runtime_error,
        "Invalid enum value (zoo) for type ns::StringEnum"
    );

    EXPECT_EQ(std::size(ns::kStringEnumValues), 3);

    ns::StringEnum values[] = {ns::StringEnum::kFoo, ns::StringEnum::kBar, ns::StringEnum::kSomeThing};
    std::size_t index = 0;
    for (const auto& value : ns::kStringEnumValues) {
        EXPECT_EQ(value, values[index]);
        ++index;
    }
}

TEST(Simple, AllOf) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    auto obj = json.As<ns::AllOf>();

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, OneOf) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::OneOf>();

    EXPECT_EQ(std::get<ns::OneOf__O2>(obj), ns::OneOf__O2());

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, OneOfWithDiscriminator) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectFoo", "foo", 42));
    auto obj = json.As<ns::ObjectOneOfWithDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "ObjectFoo");
    EXPECT_EQ(std::get<0>(obj.oneof.value()).foo, 42);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, Indirect) {
    auto json = formats::json::MakeObject(
        "data",
        "smth",
        "left",
        formats::json::MakeObject("data", "left"),
        "right",
        formats::json::MakeObject("data", "right", "left", formats::json::MakeObject("data", "rightleft"))
    );

    auto obj = json.As<ns::TreeNode>();
    EXPECT_EQ(obj.data, "smth");
    EXPECT_EQ(obj.left, (ns::TreeNode{"left", std::nullopt, std::nullopt}));
    EXPECT_EQ(obj.right, (ns::TreeNode{"right", ns::TreeNode{"rightleft", std::nullopt, std::nullopt}, std::nullopt}));
}

TEST(Simple, HyphenField) {
    ns::ObjectWithHyphenField obj;
    EXPECT_EQ(obj.foo_field, std::nullopt);
}

TEST(Simple, SubSubObjectSmoke) { [[maybe_unused]] ns::Objectx::Objectx_::Objectx__ x; }

TEST(Simple, ExtraType) {
    static_assert(std::is_same_v<
                  decltype(std::declval<ns::ObjectWithExtraType>().extra),
                  std::map<std::string, std::string>>);
}

TEST(Simple, CppName) {
    ns::ObjectName obj;
    EXPECT_EQ(obj.bar, std::nullopt);
}

TEST(Simple, Date) {
    auto json = formats::json::MakeObject("created_at", "2020-10-01");
    auto obj = json.As<ns::ObjectDate>();

    EXPECT_EQ(obj.created_at, utils::datetime::Date(2020, 10, 01));
}

TEST(Simple, DateTime) {
    auto date = "2020-10-01T12:34:56+12:34";
    auto json = formats::json::MakeObject("updated_at", date);
    auto obj = json.As<ns::ObjectDate>();

    utils::datetime::TimePointTz tp{
        utils::datetime::Stringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.updated_at, tp);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["updated_at"].As<std::string>();
    EXPECT_EQ(str, date);
}

TEST(Simple, DateTimeExtra) {
    auto date = "2020-10-01T12:34:56+12:34";
    auto json = formats::json::MakeObject("updated_at_extra", date);
    auto obj = json.As<ns::ObjectDate>();

    utils::datetime::TimePointTz tp{
        utils::datetime::Stringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.updated_at_extra->time_point, tp);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["updated_at_extra"].As<std::string>();
    EXPECT_EQ(str, date);
}

TEST(Simple, DateTimeIsoBasic) {
    auto date = "2020-10-01T12:34:56+1234";
    auto json = formats::json::MakeObject("deleted_at", date);
    auto obj = json.As<ns::ObjectDate>();

    utils::datetime::TimePointTzIsoBasic tp{
        utils::datetime::Stringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.deleted_at, tp);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["deleted_at"].As<std::string>();
    EXPECT_EQ(str, date);
}

TEST(Simple, Uuid) {
    auto uuid = "01234567-89ab-cdef-0123-456789abcdef";
    auto json = formats::json::MakeObject("uuid", uuid);
    auto obj = json.As<ns::ObjectUuid>();

    boost::uuids::string_generator gen;
    boost::uuids::uuid expected = gen(uuid);
    EXPECT_EQ(obj.uuid, expected);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["uuid"].As<std::string>();
    EXPECT_EQ(str, uuid);
}

USERVER_NAMESPACE_END
