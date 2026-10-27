#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/may_parse.hpp>
#include <userver/formats/parse/variant.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [Sample formats::parse::MayParseAs customization]
struct ObjectOnly {};

ObjectOnly Parse(const formats::json::Value& value, formats::parse::To<ObjectOnly>) {
    value.CheckObject();
    return ObjectOnly{};
}

bool MayParseAs(const formats::json::Value& value, formats::parse::To<ObjectOnly>) { return value.IsObject(); }
/// [Sample formats::parse::MayParseAs customization]

struct CountedObjectWithId {
    int id{};
};

int counted_object_with_id_parse_calls = 0;

CountedObjectWithId Parse(const formats::json::Value& value, formats::parse::To<CountedObjectWithId>) {
    ++counted_object_with_id_parse_calls;
    return CountedObjectWithId{value["id"].As<int>()};
}

bool MayParseAs(const formats::json::Value& value, formats::parse::To<CountedObjectWithId>) { return value.IsObject(); }

struct TypeTagged {};

TypeTagged Parse(const formats::json::Value& value, formats::parse::To<TypeTagged>) {
    value.CheckObject();
    return TypeTagged{};
}

bool MayParseAs(const formats::json::Value& value, formats::parse::To<TypeTagged>) {
    return value["type"].As<std::string>() == "tagged";
}

template <typename T>
std::string GetParseErrorMessage(const formats::json::Value& value) {
    try {
        value.As<T>();
    } catch (const formats::json::ParseException& e) {
        return e.what();
    }
    return {};
}

}  // namespace

TEST(FormatsJsonMayParseAs, BuiltInChecks) {
    const auto json = formats::json::FromString(R"~({"string": "a", "bool": true, "int": 1, "double": 1.5})~");

    EXPECT_TRUE(formats::parse::MayParseAs<std::string>(json["string"]));
    EXPECT_FALSE(formats::parse::MayParseAs<std::string>(json["int"]));
    EXPECT_FALSE(formats::parse::MayParseAs<std::string>(json["missing"]));

    EXPECT_TRUE(formats::parse::MayParseAs<bool>(json["bool"]));
    EXPECT_FALSE(formats::parse::MayParseAs<bool>(json["string"]));

    EXPECT_TRUE(formats::parse::MayParseAs<int>(json["int"]));
    EXPECT_TRUE(formats::parse::MayParseAs<double>(json["double"]));
    EXPECT_FALSE(formats::parse::MayParseAs<int>(json["bool"]));
    EXPECT_FALSE(formats::parse::MayParseAs<double>(json["missing"]));
}

TEST(FormatsJsonMayParseAs, TypesWithoutChecksMayParse) {
    const auto json = formats::json::FromString(R"~({"int": 1})~");

    EXPECT_TRUE(formats::parse::MayParseAs<std::vector<int>>(json["int"]));
    EXPECT_TRUE(formats::parse::MayParseAs<std::string>(formats::yaml::FromString("1")));
}

TEST(FormatsJsonMayParseAs, OverloadFoundByAdl) {
    const auto json = formats::json::FromString(R"~({"object": {}, "int": 1})~");

    EXPECT_TRUE(formats::parse::MayParseAs<ObjectOnly>(json["object"]));
    EXPECT_NO_THROW(json["object"].As<ObjectOnly>());

    EXPECT_FALSE(formats::parse::MayParseAs<ObjectOnly>(json["int"]));
    EXPECT_THROW(json["int"].As<ObjectOnly>(), formats::json::TypeMismatchException);
}

TEST(FormatsJsonMayParseAs, ThrowingOverloadMeansMayParse) {
    const auto json = formats::json::FromString(R"~({"string": "abc"})~");

    EXPECT_TRUE(formats::parse::MayParseAs<TypeTagged>(json["string"]));

    const auto parsed = json["string"].As<std::variant<TypeTagged, std::string>>();
    EXPECT_EQ(std::get<std::string>(parsed), "abc");
}

TEST(FormatsJsonMayParseAs, VariantSkipsRejectedAlternatives) {
    const auto json = formats::json::FromString(R"~({"int": 1})~");

    counted_object_with_id_parse_calls = 0;
    const auto parsed = json["int"].As<std::variant<CountedObjectWithId, int>>();

    EXPECT_EQ(std::get<int>(parsed), 1);
    EXPECT_EQ(counted_object_with_id_parse_calls, 0);
}

TEST(FormatsJsonMayParseAs, VariantParsesEachAlternativeOnceOnFailure) {
    using Variant = std::variant<int, CountedObjectWithId>;
    const auto json = formats::json::FromString(R"~({"object": {}})~");

    counted_object_with_id_parse_calls = 0;
    UEXPECT_THROW_MSG(json["object"].As<Variant>(), formats::json::ParseException, "Failed to parse as int");

    EXPECT_EQ(counted_object_with_id_parse_calls, 1);
}

TEST(FormatsJsonMayParseAs, VariantErrorKeepsAlternativesOrder) {
    const auto json = formats::json::FromString(R"~({"object": {}})~");

    const auto message = GetParseErrorMessage<std::variant<int, CountedObjectWithId>>(json["object"]);

    const auto object_error = message.find("CountedObjectWithId:");
    ASSERT_NE(object_error, std::string::npos);
    EXPECT_LT(message.find("Failed to parse as int:"), object_error);
}

USERVER_NAMESPACE_END
