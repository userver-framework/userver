#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/object.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

TEST(FormatsJsonObject, ThrowsOnNonObjectValue) {
    using formats::json::FromString;

    auto json = FromString("[1, 2]");

    EXPECT_THROW(formats::json::Object{json}, formats::json::TypeMismatchException);
    EXPECT_THROW(formats::json::Object{std::move(json)}, formats::json::TypeMismatchException);
    EXPECT_THROW(
        formats::json::Object{formats::json::ValueBuilder(formats::common::Type::kNull)},
        formats::json::TypeMismatchException
    );
}

TEST(FormatsJsonObject, EmptyByDefault) {
    ASSERT_NO_THROW(formats::json::Object{});

    formats::json::Object object;

    ASSERT_TRUE(object.GetValue().IsObject());
    EXPECT_TRUE(object.IsEmpty());
    EXPECT_EQ(object.begin(), object.end());
    EXPECT_EQ(object.GetSize(), std::size_t{0});
    EXPECT_TRUE(object.IsRoot());
    EXPECT_EQ(object.GetPath(), "/");
}

TEST(FormatsJsonObject, CanBeParsedFromValue) {
    using formats::json::FromString;

    auto json = FromString(R"({
      "object": {
        "key1": 1,
        "key2": "hello"
      }
    })");
    auto object_value = FromString(R"({"key3": 3})");

    formats::json::Object object;

    ASSERT_NO_THROW(object = json["object"].As<formats::json::Object>());
    ASSERT_TRUE(object.GetValue().IsObject());
    EXPECT_FALSE(object.IsRoot());
    EXPECT_EQ(object.GetPath(), "object");
    EXPECT_FALSE(object.IsEmpty());
    ASSERT_EQ(object.GetSize(), 2);
    ASSERT_TRUE(object.HasMember("key1"));
    ASSERT_TRUE(object.HasMember("key2"));
    EXPECT_EQ(object["key1"].As<int>(), 1);
    EXPECT_EQ(object["key2"].As<std::string>(), "hello");

    ASSERT_NO_THROW(object = formats::json::Object{object_value});
    ASSERT_NE(object.begin(), object.end());
    EXPECT_EQ((*object.begin()).As<int>(), 3);

    ASSERT_NO_THROW(
        object = json["non_existent"].As<formats::json::Object>(formats::json::Value::DefaultConstructed{})
    );
    EXPECT_TRUE(object.IsEmpty());

    ASSERT_NO_THROW(object = json["non_existent"].As<formats::json::Object>(object_value));
    ASSERT_EQ(object.GetSize(), std::size_t{1});
    ASSERT_TRUE(object.HasMember("key3"));
    EXPECT_EQ(object["key3"].As<int>(), 3);
}

TEST(FormatsJsonObject, CanBeSerializedToBuilder) {
    using formats::json::FromString;
    const formats::json::Object object{FromString(R"({
      "key1": 1,
      "key2": "hello"
    })")};

    formats::json::ValueBuilder builder;
    builder["object1"] = object;
    builder["object2"] = formats::json::Object{FromString(R"({"key3": 3})")};
    const auto json = builder.ExtractValue();

    ASSERT_TRUE(json["object1"].IsObject());
    ASSERT_TRUE(json["object2"].IsObject());
    EXPECT_EQ(json["object1"]["key1"].As<int>(), 1);
    EXPECT_EQ(json["object1"]["key2"].As<std::string>(), "hello");
    EXPECT_EQ(json["object2"]["key3"].As<int>(), 3);
}

TEST(FormatsJsonObject, CanBeComparedForEquality) {
    using formats::json::FromString;
    const formats::json::Object object1{FromString(R"({
      "key1": 1,
      "key2": "hello"
    })")};
    const formats::json::Object object2{FromString(R"({
      "key1": 1,
      "key2": "hello1"
    })")};
    const formats::json::Object object3{FromString(R"({
      "key2": "hello",
      "key1": 1
    })")};
    const formats::json::Object object4;

    EXPECT_TRUE(object1 == object1);
    EXPECT_FALSE(object1 == object2);
    EXPECT_FALSE(object1 == object2.GetValue());
    EXPECT_FALSE(object1.GetValue() == object2);
    EXPECT_TRUE(object1 == object3);
    EXPECT_TRUE(object1 == object3.GetValue());
    EXPECT_TRUE(object1.GetValue() == object3);
    EXPECT_FALSE(object1 == object4);

    EXPECT_FALSE(object1 != object1);
    EXPECT_TRUE(object1 != object2);
    EXPECT_TRUE(object1 != object2.GetValue());
    EXPECT_TRUE(object1.GetValue() != object2);
    EXPECT_FALSE(object1 != object3);
    EXPECT_FALSE(object1 != object3.GetValue());
    EXPECT_FALSE(object1.GetValue() != object3);
    EXPECT_TRUE(object1 != object4);
}

TEST(FormatsJsonObject, CanBeCloned) {
    using formats::json::FromString;
    const formats::json::Object object{FromString(R"({"key1": 1})")};
    formats::json::Object clone{object.Clone()};

    EXPECT_EQ(object, clone);
}

TEST(FormatsJsonObject, CanBeConvertedToContainers) {
    using formats::json::FromString;
    using MapType = std::unordered_map<std::string, std::string>;

    const formats::json::Object object{FromString(R"({"key": "value"})")};
    MapType expected{{"key", "value"}};

    EXPECT_EQ(object.As<MapType>(), expected);
    EXPECT_EQ(object.ConvertTo<MapType>(), expected);
}

USERVER_NAMESPACE_END
