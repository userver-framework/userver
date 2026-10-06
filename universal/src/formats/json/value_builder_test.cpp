#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/death_tests.hpp>
#include <userver/utils/string_literal.hpp>

// for testing std::optional/null
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include <formats/common/value_builder_test.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct InstantiationDeathTest<formats::json::ValueBuilder> : public ::testing::Test {
    using ValueBuilder = formats::json::ValueBuilder;

    using Exception = formats::json::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, InstantiationDeathTest, formats::json::ValueBuilder);
INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, CommonValueBuilderTests, formats::json::ValueBuilder);

namespace {

template <typename T>
auto JsonAs(T expected) {
    return testing::ResultOf(
        [](const formats::json::Value& value) { return value.As<T>(); },
        testing::Eq(std::move(expected))
    );
}

auto JsonAs(const char* expected) { return JsonAs(std::string{expected}); }

MATCHER(IsJsonNull, "") { return arg.IsNull(); }

auto JsonElements(const formats::json::Value& value) {
    return std::vector<formats::json::Value>(value.begin(), value.end());
}

template <typename Expected>
bool MatchesValueMember(
    const formats::json::Value::const_iterator& member,
    const Expected& expected,
    testing::MatchResultListener* result_listener
) {
    if (member.GetName() != "value") {
        *result_listener << "whose member name is " << testing::PrintToString(member.GetName());
        return false;
    }
    return testing::ExplainMatchResult(JsonAs(expected), *member, result_listener);
}

MATCHER_P(IsJsonObjectWithValue, expected, "") {
    return arg.IsObject() && arg.GetSize() == 1 && MatchesValueMember(arg.begin(), expected, result_listener);
}

MATCHER_P2(IsJsonObjectWithDuplicateValues, first, second, "") {
    if (!arg.IsObject() || arg.GetSize() != 2) {
        return false;
    }

    auto member = arg.begin();
    if (!MatchesValueMember(member, first, result_listener)) {
        return false;
    }
    return MatchesValueMember(++member, second, result_listener);
}

template <typename Float>
void TestPushBackNonFinite(Float value) {
#ifdef NDEBUG
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    EXPECT_THROW(builder.PushBack(value), formats::json::Exception);
#else
    const auto push_back = [value] {
        formats::json::ValueBuilder builder(formats::common::Type::kArray);
        builder.PushBack(value);
    };
    UEXPECT_DEATH(push_back(), "");
#endif
}

}  // namespace

TEST(JsonValueBuilder, ExampleUsage) {
    /// [Sample formats::json::ValueBuilder usage]
    // #include <userver/formats/json.hpp>
    formats::json::ValueBuilder builder;
    builder["key1"] = 1;
    builder["key2"]["key3"] = "val";
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json["key1"].As<int>(), 1);
    ASSERT_EQ(json["key2"]["key3"].As<std::string>(), "val");
    /// [Sample formats::json::ValueBuilder usage]
}

TEST(JsonValueBuilder, ValueString) {
    formats::json::ValueBuilder builder;
    builder = "abc";
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json.As<std::string>(), "abc");
    ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueEmptyString) {
    formats::json::ValueBuilder builder;
    builder = "";
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json.As<std::string>(), "");
    ASSERT_THROW(json.As<std::vector<std::string>>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueNumber) {
    formats::json::ValueBuilder builder;
    builder = 321;
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json.As<int>(), 321);
    ASSERT_THROW(json.As<bool>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueTrue) {
    formats::json::ValueBuilder builder;
    builder = true;
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json.As<bool>(), true);
    ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueFalse) {
    formats::json::ValueBuilder builder = false;
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json.As<bool>(), false);
    ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueNull) {
    formats::json::ValueBuilder builder;
    builder = std::optional<int>{};
    const formats::json::Value json = builder.ExtractValue();

    ASSERT_EQ(json.As<std::optional<int>>(), std::nullopt);
    ASSERT_EQ(json.As<std::optional<std::vector<std::string>>>(), std::nullopt);
    ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);

    formats::json::ValueBuilder builder_def;
    const formats::json::Value json_def = builder_def.ExtractValue();

    ASSERT_EQ(json_def.As<std::optional<std::string>>(), std::nullopt);
}

TEST(JsonValueBuilder, ReserveArray) {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    builder.PushBack(1);
    auto element = builder[0];

    builder.Reserve(100);
    EXPECT_GE(builder.GetCapacity(), 100);
    element = 2;

    const auto json = builder.ExtractValue();
    ASSERT_EQ(json.GetSize(), 1);
    EXPECT_EQ(json[0].As<int>(), 2);
}

TEST(JsonValueBuilder, ReserveObject) {
    formats::json::ValueBuilder builder(formats::common::Type::kObject);
    builder["key"] = 1;
    auto member = builder["key"];

    builder.Reserve(100);
    EXPECT_GE(builder.GetCapacity(), 100);
    member = 2;

    const auto json = builder.ExtractValue();
    ASSERT_EQ(json.GetSize(), 1);
    EXPECT_EQ(json["key"].As<int>(), 2);
}

TEST(JsonValueBuilder, ReserveRejectsNonContainer) {
    formats::json::ValueBuilder null_builder;
    formats::json::ValueBuilder scalar_builder = 1;

    EXPECT_THROW(null_builder.Reserve(1), formats::json::TypeMismatchException);
    EXPECT_THROW(scalar_builder.Reserve(1), formats::json::TypeMismatchException);
    EXPECT_THROW(null_builder.GetCapacity(), formats::json::TypeMismatchException);
    EXPECT_THROW(scalar_builder.GetCapacity(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, PushBackBracedValues) {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    builder.PushBack({});
    builder.PushBack({0});
    builder.PushBack({0u});
    builder.PushBack({"const char pointer"});
    builder.PushBack({std::string{"foo"}});
    builder.PushBack({std::string_view{"string view"}});
    builder.PushBack({utils::StringLiteral{"string literal"}});
    builder.PushBack({formats::json::ValueBuilder{1}});

    const auto value = builder.ExtractValue();
    EXPECT_THAT(
        JsonElements(value),
        testing::ElementsAre(
            IsJsonNull(),
            JsonAs(0),
            JsonAs(0u),
            JsonAs("const char pointer"),
            JsonAs("foo"),
            JsonAs("string view"),
            JsonAs("string literal"),
            JsonAs(1)
        )
    );
}

TEST(JsonValueBuilder, PushBackRejectsMultipleBracedValues) {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);

    EXPECT_THROW(builder.PushBack({1, 2}), formats::json::Exception);
    EXPECT_TRUE(builder.IsEmpty());
}

TEST(JsonValueBuilder, PushBackRvalueSources) {
    formats::json::ValueBuilder destination(formats::common::Type::kArray);
    formats::json::ValueBuilder builder_source = 1;
    auto value_source = formats::json::ValueBuilder{2}.ExtractValue();

    destination.PushBack(std::move(builder_source));
    destination.PushBack(std::move(value_source));

    EXPECT_EQ(destination.ExtractValue().As<std::vector<int>>(), (std::vector<int>{1, 2}));
}

TEST(JsonValueBuilder, PushBackWrongTypeRejectsRvalueSources) {
    formats::json::ValueBuilder destination = 0;
    auto source_value = formats::json::ValueBuilder{2}.ExtractValue();

    EXPECT_THROW(destination.PushBack(formats::json::ValueBuilder{1}), formats::json::TypeMismatchException);
    EXPECT_THROW(destination.PushBack(std::move(source_value)), formats::json::TypeMismatchException);

    EXPECT_EQ(destination.ExtractValue().As<int>(), 0);
    // source_value must remain unchanged on a type-mismatch exception.
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_EQ(source_value.As<int>(), 2);
}

TEST(JsonValueBuilder, PushBackConcreteTypes) {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    char mutable_string[] = "mutable string";
    const std::string string = "string";

    builder.PushBack(nullptr);
    builder.PushBack(false);
    builder.PushBack("const char pointer");
    builder.PushBack(mutable_string);
    builder.PushBack(string);
    builder.PushBack(std::string_view{"string view"});
    builder.PushBack(0);
    builder.PushBack(0u);
    builder.PushBack(std::uint64_t{0});
    builder.PushBack(std::int64_t{0});
    builder.PushBack(0.0f);
    builder.PushBack(0.0);

    const auto value = builder.ExtractValue();
    EXPECT_THAT(
        JsonElements(value),
        testing::ElementsAre(
            IsJsonNull(),
            JsonAs(false),
            JsonAs("const char pointer"),
            JsonAs("mutable string"),
            JsonAs("string"),
            JsonAs("string view"),
            JsonAs(0),
            JsonAs(0u),
            JsonAs(std::uint64_t{0}),
            JsonAs(std::int64_t{0}),
            JsonAs(0.0f),
            JsonAs(0.0)
        )
    );
}

TEST(JsonValueBuilder, PushBackPromotedArithmeticTypes) {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);

    builder.PushBack(static_cast<signed char>(-1));
    builder.PushBack(static_cast<unsigned char>(2));
    builder.PushBack(static_cast<short>(-3));
    builder.PushBack(static_cast<unsigned short>(4));
    builder.PushBack(-5LL);
    builder.PushBack(6ULL);
    builder.PushBack(7.5L);

    const auto value = builder.ExtractValue();
    EXPECT_THAT(
        JsonElements(value),
        testing::ElementsAre(
            JsonAs(-1),
            JsonAs(2u),
            JsonAs(-3),
            JsonAs(4u),
            JsonAs(std::int64_t{-5}),
            JsonAs(std::uint64_t{6}),
            JsonAs(7.5)
        )
    );
}

TEST(JsonValueBuilder, PushBackArithmeticKeepsElementBuilderValidAfterReallocation) {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    builder.Reserve(1);
    builder.PushBack(1);
    auto first_element = builder[0];

    builder.PushBack(2);
    first_element = 3;

    const auto value = builder.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(JsonAs(3), JsonAs(2)));
}

TEST(JsonValueBuilder, PushBackRejectsNonFiniteNumbers) {
    TestPushBackNonFinite(std::numeric_limits<float>::quiet_NaN());
    TestPushBackNonFinite(std::numeric_limits<float>::infinity());
    TestPushBackNonFinite(std::numeric_limits<double>::quiet_NaN());
    TestPushBackNonFinite(std::numeric_limits<double>::infinity());
}

TEST(JsonValueBuilder, PushBackValuePreservesBorrowedSource) {
    formats::json::ValueBuilder source(formats::common::Type::kObject);
    source["nested"]["value"] = 1;
    const auto source_value = source.ExtractValue();
    // Keep an owning alias to exercise copying from shared storage.
    const auto source_alias = source_value;  // NOLINT(performance-unnecessary-copy-initialization)

    formats::json::ValueBuilder result(formats::common::Type::kArray);
    result.Reserve(1);
    result.PushBack(source_value["nested"]);

    const auto value = result.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(IsJsonObjectWithValue(1)));
    EXPECT_EQ(source_value, source_alias);
    EXPECT_FALSE(value[0].DebugIsReferencingSameMemory(source_value["nested"]));
}

TEST(JsonValueBuilder, PushBackValueMovesUniqueRootAndCopiesAlias) {
    formats::json::ValueBuilder unique_source(formats::common::Type::kObject);
    unique_source["value"] = 1;
    auto unique_value = unique_source.ExtractValue();

    formats::json::ValueBuilder result(formats::common::Type::kArray);
    result.PushBack(std::move(unique_value));

    formats::json::ValueBuilder aliased_source(formats::common::Type::kObject);
    aliased_source["value"] = 2;
    auto aliased_value = aliased_source.ExtractValue();
    const auto alias = aliased_value;
    result.PushBack(std::move(aliased_value));

    const auto value = result.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(IsJsonObjectWithValue(1), IsJsonObjectWithValue(2)));
    EXPECT_EQ(alias["value"].As<int>(), 2);
    EXPECT_FALSE(value[1].DebugIsReferencingSameMemory(alias));
}

TEST(JsonValueBuilder, PushBackValuePreservesDuplicateMembers) {
    formats::json::ValueBuilder duplicate(formats::common::Type::kObject);
    duplicate.EmplaceNocheck("value", 1);
    duplicate.EmplaceNocheck("value", 2);
    const auto duplicate_value = duplicate.ExtractValue();

    formats::json::ValueBuilder result(formats::common::Type::kArray);
    result.PushBack(duplicate_value);

    const auto value = result.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(IsJsonObjectWithDuplicateValues(1, 2)));
}

/// [Sample Customization formats::json::ValueBuilder usage]
namespace my_namespace {

struct MyKeyValue {
    std::string field1;
    int field2;
};

// The function must be declared in the namespace of your type
// Keep external linkage in this public documentation snippet.
// NOLINTNEXTLINE(misc-use-internal-linkage)
formats::json::Value Serialize(const MyKeyValue& data, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder builder;
    builder["field1"] = data.field1;
    builder["field2"] = data.field2;

    return builder.ExtractValue();
}

TEST(JsonValueBuilder, ExampleCustomization) {
    const MyKeyValue object = {.field1 = "val", .field2 = 1};
    formats::json::ValueBuilder builder;
    builder["example"] = object;
    auto json = builder.ExtractValue();
    ASSERT_EQ(json["example"]["field1"].As<std::string>(), "val");
    ASSERT_EQ(json["example"]["field2"].As<int>(), 1);
}

TEST(JsonValueBuilder, StringViewRemove) {
    formats::json::ValueBuilder builder;
    const std::string str = "ab";
    builder["a"] = 1;
    builder[str] = 2;
    builder.Remove(std::string_view(str.data(), 1));

    EXPECT_EQ(1, builder.GetSize());

    const auto value = builder.ExtractValue();
    EXPECT_EQ(2, value[str].As<int>());
}

TEST(JsonValueBuilder, StringViewHasMember) {
    formats::json::ValueBuilder main_builder;
    const std::string str = "ab";
    main_builder[str] = 2;
    EXPECT_EQ(false, main_builder.HasMember("a"));
    EXPECT_EQ(false, main_builder.HasMember(std::string_view(str.data(), 1)));
    EXPECT_EQ(true, main_builder.HasMember(std::string_view(str.data(), 2)));
}

TEST(JsonValueBuilder, StringViewEmplaceNocheck) {
    formats::json::ValueBuilder main_builder;
    const std::string str = "ab";
    main_builder.EmplaceNocheck(std::string_view(str.data(), 1), 1);
    main_builder.EmplaceNocheck(std::string_view(str.data(), 2), 2);
    main_builder.EmplaceNocheck(std::string_view(std::string(1024, 'a') + 'b'), 1025);

    const auto value = main_builder.ExtractValue();
    EXPECT_EQ(1, value["a"].As<int>());
    EXPECT_EQ(2, value["ab"].As<int>());
    EXPECT_EQ(1025, value[std::string(1024, 'a') + 'b'].As<int>());
}

}  // namespace my_namespace

/// [Sample Customization formats::json::ValueBuilder usage]

USERVER_NAMESPACE_END
