#include <gtest/gtest.h>

#include <userver/formats/json/array.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

TEST(FormatsJsonArray, ThrowsOnNonArrayValue) {
    using formats::json::FromString;

    auto json = FromString(R"({
      "key": "value"
    })");

    EXPECT_THROW(formats::json::Array{json}, formats::json::TypeMismatchException);
    EXPECT_THROW(formats::json::Array{std::move(json)}, formats::json::TypeMismatchException);
    EXPECT_THROW(
        formats::json::Array{formats::json::ValueBuilder(formats::common::Type::kNull)},
        formats::json::TypeMismatchException
    );
}

TEST(FormatsJsonArray, EmptyByDefault) {
    ASSERT_NO_THROW(formats::json::Array{});

    formats::json::Array array;

    ASSERT_TRUE(array.GetValue().IsArray());
    EXPECT_TRUE(array.IsEmpty());
    EXPECT_EQ(array.begin(), array.end());
    EXPECT_EQ(array.rbegin(), array.rend());
    EXPECT_EQ(array.GetSize(), std::size_t{0});
    EXPECT_THROW(array.CheckInBounds(std::size_t{1}), formats::json::OutOfBoundsException);
    EXPECT_TRUE(array.IsRoot());
    EXPECT_EQ(array.GetPath(), "/");
}

TEST(FormatsJsonArray, CanBeParsedFromValue) {
    using formats::json::FromString;

    auto json = FromString(R"({
      "array": [1, 2, 3]
    })");
    auto array_value = FromString("[5]");

    formats::json::Array array;

    ASSERT_NO_THROW(array = json["array"].As<formats::json::Array>());
    ASSERT_TRUE(array.GetValue().IsArray());
    EXPECT_FALSE(array.IsRoot());
    EXPECT_EQ(array.GetPath(), "array");
    EXPECT_FALSE(array.IsEmpty());
    EXPECT_NO_THROW(array.CheckInBounds(std::size_t{1}));
    ASSERT_EQ(array.GetSize(), 3);
    EXPECT_EQ(array[0].As<int>(), 1);
    EXPECT_EQ(array[1].As<int>(), 2);
    EXPECT_EQ(array[2].As<int>(), 3);
    ASSERT_NE(array.begin(), array.end());
    EXPECT_EQ((*array.begin()).As<int>(), 1);
    ASSERT_NE(array.rbegin(), array.rend());
    EXPECT_EQ((*array.rbegin()).As<int>(), 3);

    ASSERT_NO_THROW(array = json["non_existent"].As<formats::json::Array>(formats::json::Value::DefaultConstructed{}));
    EXPECT_TRUE(array.IsEmpty());

    ASSERT_NO_THROW(array = json["non_existent"].As<formats::json::Array>(array_value));
    ASSERT_EQ(array.GetSize(), std::size_t{1});
    EXPECT_EQ(array[0].As<int>(), 5);
}

TEST(FormatsJsonArray, CanBeSerializedToBuilder) {
    using formats::json::FromString;
    const formats::json::Array array{FromString("[1, 2, 3]")};

    formats::json::ValueBuilder builder;
    builder["array1"] = array;
    builder["array2"] = formats::json::Array{FromString("[1001, 1002, 1003]")};
    const auto json = builder.ExtractValue();

    ASSERT_TRUE(json["array1"].IsArray());
    ASSERT_TRUE(json["array2"].IsArray());
    EXPECT_EQ(json["array1"].As<std::vector<int>>(), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(json["array2"].As<std::vector<int>>(), (std::vector<int>{1001, 1002, 1003}));
}

TEST(FormatsJsonArray, CanBeComparedForEquality) {
    using formats::json::FromString;
    const formats::json::Array array1{FromString("[1, 2, 3]")};
    const formats::json::Array array2{FromString("[1, 3, 2]")};
    const formats::json::Array array3{FromString("[1, 2, 3]")};
    const formats::json::Array array4;

    EXPECT_TRUE(array1 == array1);
    EXPECT_FALSE(array1 == array2);
    EXPECT_FALSE(array1 == array2.GetValue());
    EXPECT_FALSE(array1.GetValue() == array2);
    EXPECT_TRUE(array1 == array3);
    EXPECT_TRUE(array1 == array3.GetValue());
    EXPECT_TRUE(array1.GetValue() == array3);
    EXPECT_FALSE(array1 == array4);

    EXPECT_FALSE(array1 != array1);
    EXPECT_TRUE(array1 != array2);
    EXPECT_TRUE(array1 != array2.GetValue());
    EXPECT_TRUE(array1.GetValue() != array2);
    EXPECT_FALSE(array1 != array3);
    EXPECT_FALSE(array1 != array3.GetValue());
    EXPECT_FALSE(array1.GetValue() != array3);
    EXPECT_TRUE(array1 != array4);
}

TEST(FormatsJsonArray, CanBeCloned) {
    using formats::json::FromString;
    const formats::json::Array array{FromString("[1, 2, 3]")};
    formats::json::Array clone{array.Clone()};

    EXPECT_EQ(array, clone);
}

TEST(FormatsJsonArray, CanBeConvertedToContainers) {
    using formats::json::FromString;
    const formats::json::Array array{FromString("[1, 2, 3]")};
    const std::vector<int> expected{1, 2, 3};

    EXPECT_EQ(array.As<std::vector<int>>(), expected);
    EXPECT_EQ(array.ConvertTo<std::vector<int>>(), expected);
}

USERVER_NAMESPACE_END
