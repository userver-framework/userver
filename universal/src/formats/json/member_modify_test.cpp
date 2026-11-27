#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/serialize/to.hpp>

#include <formats/common/member_modify_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

template <typename Character>
requires(std::is_same_v<Character, char32_t>)
formats::json::Value Serialize(Character, To<formats::json::Value>) {
    return formats::json::ValueBuilder{"explicit-character-serializer"}.ExtractValue();
}

}  // namespace formats::serialize

namespace {

formats::json::Value BuildDefaultDecimalObject(
    const decimal64::Decimal<4>& raw,
    const std::optional<decimal64::Decimal<4>>& optional
) {
    formats::json::ValueBuilder builder;
    builder["raw"] = raw;
    builder["optional"] = optional;
    builder["empty"] = std::optional<decimal64::Decimal<4>>{};
    return builder.ExtractValue();
}

TEST(JsonValueBuilder, DecimalSerializationUsesDefaultOverloads) {
    const decimal64::Decimal<4> raw{"42"};
    const std::optional<decimal64::Decimal<4>> optional{decimal64::Decimal<4>{"32"}};

    EXPECT_EQ(
        BuildDefaultDecimalObject(raw, optional),
        formats::json::MakeObject("raw", "42", "optional", "32", "empty", nullptr)
    );
}

TEST(JsonValueBuilder, NativeCharacterOperationsPreserveCodeUnits) {
    constexpr char32_t kCharacter = U'x';
    constexpr auto kCodeUnit = static_cast<std::int64_t>(kCharacter);
    const auto explicitly_serialized =
        formats::serialize::Serialize(kCharacter, formats::serialize::To<formats::json::Value>{});
    EXPECT_EQ(explicitly_serialized.As<std::string>(), "explicit-character-serializer");
    EXPECT_EQ(formats::json::ValueBuilder{kCharacter}.ExtractValue().As<std::int64_t>(), kCodeUnit);
    formats::json::ValueBuilder object;
    object["assigned"] = kCharacter;
    object.EmplaceNocheck("emplaced", kCharacter);
    formats::json::ValueBuilder array{formats::json::Type::kArray};
    array.PushBack(kCharacter);
    const auto object_value = object.ExtractValue();
    EXPECT_EQ(object_value["assigned"].As<std::int64_t>(), kCodeUnit);
    EXPECT_EQ(object_value["emplaced"].As<std::int64_t>(), kCodeUnit);
    EXPECT_EQ(array.ExtractValue()[0].As<std::int64_t>(), kCodeUnit);
}

const auto kDoc = formats::json::MakeObject(
    "key1",
    1,
    "key2",
    "val",
    "key3",
    formats::json::MakeObject("sub", -1),
    "key4",
    formats::json::MakeArray(1, 2, 3),
    "key5",
    10.5
);

}  // namespace

template <>
struct MemberModify<formats::json::ValueBuilder> : public ::testing::Test {
    // XXX: not working from base class in clang-7 (decltype?)
    static_assert(
        std::is_assignable_v<
            decltype(*std::declval<formats::json::ValueBuilder>().begin()),
            formats::json::ValueBuilder>,
        "ValueBuilder iterators are assignable"
    );

    MemberModify()
        : builder(kDoc)
    {}

    static formats::json::Value GetValue(formats::json::ValueBuilder& bld) {
        auto v = bld.ExtractValue();
        bld = v;
        return v;
    }

    formats::json::Value GetBuiltValue() { return GetValue(builder); }

    formats::json::ValueBuilder builder;

    using ValueBuilder = formats::json::ValueBuilder;
    using Value = formats::json::Value;
    using Type = formats::json::Type;

    using ParseException = formats::json::ParseException;
    using TypeMismatchException = formats::json::TypeMismatchException;
    using OutOfBoundsException = formats::json::OutOfBoundsException;
    using MemberMissingException = formats::json::MemberMissingException;
    using Exception = formats::json::Exception;

    constexpr static auto kFromString = formats::json::FromString;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, MemberModify, formats::json::ValueBuilder);

namespace {

class JsonInlineBuilder : public ::testing::TestWithParam<bool> {
protected:
    formats::json::Value Wrap(formats::json::Value&& value) const {
        return GetParam() ? formats::json::MakeObject("value", std::move(value))
                          : formats::json::MakeArray(std::move(value));
    }

    formats::json::Value GetElement(const formats::json::Value& container) const {
        return GetParam() ? container["value"] : container[0];
    }

    static constexpr std::size_t kPayloadSize = 1024;
};

TEST_P(JsonInlineBuilder, MovesUniqueRootsAndLeavesValidNullSources) {
    std::array sources{
        formats::json::MakeObject("payload", std::string(kPayloadSize, 'x')),
        formats::json::MakeArray(std::string(kPayloadSize, 'x'), 42),
        formats::json::ValueBuilder(std::string(kPayloadSize, 'x')).ExtractValue(),
        formats::json::Value{}
    };
    for (auto& source : sources) {
        const auto expected = source.Clone();
        const auto result = Wrap(std::move(source));
        EXPECT_EQ(GetElement(result), expected);
        EXPECT_TRUE(source.IsNull());
        EXPECT_FALSE(source.IsMissing());
        EXPECT_EQ(formats::json::ToString(source), "null");
    }
}

TEST_P(JsonInlineBuilder, CopiesSharedRootsWithoutChangingAliases) {
    auto source = formats::json::MakeObject("payload", std::string(kPayloadSize, 'x'));
    const auto alias = source;
    const auto result = Wrap(std::move(source));
    EXPECT_EQ(GetElement(result), alias);
    EXPECT_EQ(source, alias);
    EXPECT_FALSE(GetElement(result).DebugIsReferencingSameMemory(alias));
}

TEST_P(JsonInlineBuilder, CopiesSharedChildrenWithoutChangingParent) {
    const auto parent = formats::json::MakeObject("child", formats::json::MakeArray(1, 2, 3));
    auto child = parent["child"];
    const auto result = Wrap(std::move(child));
    EXPECT_EQ(GetElement(result), parent["child"]);
    EXPECT_EQ(child, parent["child"]);
    EXPECT_EQ(parent, formats::json::MakeObject("child", formats::json::MakeArray(1, 2, 3)));
}

TEST_P(JsonInlineBuilder, MovesUniqueChildrenWithAttachedAndDetachedPaths) {
    for (const bool drop_root_path : {false, true}) {
        auto child =
            formats::json::MakeObject("child", formats::json::MakeObject("payload", std::string(kPayloadSize, 'x')))
                ["child"];
        if (drop_root_path) {
            child.DropRootPath();
        }
        const auto expected = child.Clone();
        const auto source_path = child.GetPath();
        const auto result = Wrap(std::move(child));
        EXPECT_EQ(GetElement(result), expected);
        EXPECT_TRUE(child.IsNull());
        EXPECT_FALSE(child.IsMissing());
        EXPECT_EQ(child.GetPath(), source_path);
    }
}

TEST_P(JsonInlineBuilder, MissingValuesPreserveTheirPaths) {
    auto missing = formats::json::MakeObject("present", 1)["missing"];
    EXPECT_THROW(Wrap(std::move(missing)), formats::json::MemberMissingException);
    EXPECT_TRUE(missing.IsMissing());
    EXPECT_EQ(missing.GetPath(), "missing");
}

TEST_P(JsonInlineBuilder, CopiesLvaluePayloads) {
    const auto source = formats::json::MakeObject("payload", std::string(kPayloadSize, 'x'));
    const auto result = GetParam() ? formats::json::MakeObject("value", source) : formats::json::MakeArray(source);
    EXPECT_EQ(GetElement(result), source);
    EXPECT_FALSE(GetElement(result).DebugIsReferencingSameMemory(source));
}

INSTANTIATE_TEST_SUITE_P(ObjectAndArray, JsonInlineBuilder, ::testing::Bool());

}  // namespace

USERVER_NAMESPACE_END
