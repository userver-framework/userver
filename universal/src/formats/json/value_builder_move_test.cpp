#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::size_t kPayloadSize = 128;

class JsonValueBuilderMove : public testing::TestWithParam<formats::json::Type> {};

TEST_P(JsonValueBuilderMove, RootMoveKeepsSavedChildAttachedToSource) {
    using formats::json::Type;
    using formats::json::ValueBuilder;

    const std::string payload(kPayloadSize, 'x');
    ValueBuilder source{GetParam()};
    if (GetParam() == Type::kObject) {
        source.EmplaceNocheck("value", payload);
    } else {
        source.PushBack(payload);
    }
    auto child = GetParam() == Type::kObject ? source["value"] : source[0];

    ValueBuilder destination{std::move(source)};

    EXPECT_TRUE(source.IsNull());
    EXPECT_THROW(child = 42, formats::json::TypeMismatchException);
    if (GetParam() == Type::kObject) {
        source.EmplaceNocheck("value", "replacement");
    } else {
        source.PushBack("replacement");
    }
    child = 42;

    const auto moved = destination.ExtractValue();
    EXPECT_EQ((GetParam() == Type::kObject ? moved["value"] : moved[0]).As<std::string>(), payload);
    const auto reused = source.ExtractValue();
    EXPECT_EQ((GetParam() == Type::kObject ? reused["value"] : reused[0]).As<int>(), 42);
}

TEST(JsonValueBuilderMove, ChildMoveCopiesPayloadAndPreservesParent) {
    using formats::json::ValueBuilder;

    const std::string payload(kPayloadSize, 'x');
    ValueBuilder parent;
    auto child = parent["child"];
    child["value"] = payload;
    auto saved_value = child["value"];

    ValueBuilder destination{std::move(child)};
    destination["value"] = "changed";
    EXPECT_EQ(ValueBuilder{child}.ExtractValue()["value"].As<std::string>(), payload);
    EXPECT_EQ(ValueBuilder{parent}.ExtractValue()["child"]["value"].As<std::string>(), payload);

    saved_value = 43;
    EXPECT_EQ(parent.ExtractValue()["child"]["value"].As<int>(), 43);
    EXPECT_EQ(destination.ExtractValue()["value"].As<std::string>(), "changed");
}

TEST(JsonValueBuilderMove, UniqueRootMoveResetsSourceAndPreservesPayload) {
    using formats::json::ValueBuilder;

    const std::string payload(kPayloadSize, 'x');
    ValueBuilder source{payload};
    ValueBuilder destination{std::move(source)};

    EXPECT_TRUE(source.IsNull());
    source["value"] = 42;
    EXPECT_EQ(destination.ExtractValue().As<std::string>(), payload);
    EXPECT_EQ(source.ExtractValue()["value"].As<int>(), 42);
}

INSTANTIATE_TEST_SUITE_P(
    ArrayAndObject,
    JsonValueBuilderMove,
    testing::Values(formats::json::Type::kArray, formats::json::Type::kObject)
);

}  // namespace

USERVER_NAMESPACE_END
