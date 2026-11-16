#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if defined(__linux__)
#include <unistd.h>
#endif

#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/common/path.hpp>
#include <userver/formats/common/transfer_tag.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/formats/serialize/variant.hpp>
#include <userver/utest/death_tests.hpp>
#include <userver/utils/string_literal.hpp>

// for testing std::optional/null
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include <formats/common/value_builder_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

formats::json::Value Serialize(long double value, To<formats::json::Value>) {
    constexpr std::string_view kCustomArithmeticMember = "custom-arithmetic";
    return formats::json::MakeObject(kCustomArithmeticMember, static_cast<double>(value));
}

template <typename Value>
Value Serialize(const decimal64::Decimal<4>& number, To<Value>) {
    return typename Value::Builder{number.AsUnbiased()}.ExtractValue();
}

}  // namespace formats::serialize

template <>
struct InstantiationDeathTest<formats::json::ValueBuilder> : public ::testing::Test {
    using ValueBuilder = formats::json::ValueBuilder;

    using Exception = formats::json::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, InstantiationDeathTest, formats::json::ValueBuilder);
INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, CommonValueBuilderTests, formats::json::ValueBuilder);

namespace {

#if defined(__linux__)
constexpr int kShutdownCallbackRegistrationPriority = 101;
constexpr int kShutdownValue = 42;
constexpr auto kShutdownCheckEnvironment = "USERVER_IMPL_TEST_JSON_SHUTDOWN";
constexpr auto kShutdownCheckEnvironmentValue = "1";
constexpr auto kShutdownExecutablePath = "/proc/self/exe";

void CheckJsonAtShutdown() {
    try {
        const formats::json::Value null;
        formats::json::ValueBuilder builder;
        if (!null.IsNull() || !builder.IsNull()) {
            std::_Exit(EXIT_FAILURE);
        }
        builder["value"] = kShutdownValue;
        if (builder.ExtractValue()["value"].As<int>() != kShutdownValue) {
            std::_Exit(EXIT_FAILURE);
        }
    } catch (...) {
        std::_Exit(EXIT_FAILURE);
    }
}

struct ShutdownCallbackRegistration {
    ShutdownCallbackRegistration() {
        const auto* environment = std::getenv(kShutdownCheckEnvironment);
        if (!environment || std::string_view{environment} != kShutdownCheckEnvironmentValue) {
            return;
        }
        if (std::atexit(CheckJsonAtShutdown) != 0) {
            std::_Exit(EXIT_FAILURE);
        }
        {
            const formats::json::Value null;
            if (!null.IsNull()) {
                std::_Exit(EXIT_FAILURE);
            }
        }
        std::exit(EXIT_SUCCESS);
    }
};

[[gnu::init_priority(kShutdownCallbackRegistrationPriority
)]] const ShutdownCallbackRegistration kShutdownCallbackRegistration;

TEST(JsonValueBuilderDeathTest, SharedNullSurvivesShutdown) {
    const utest::impl::DeathTestScope scope;
    EXPECT_EXIT(
        {
            if (::setenv(kShutdownCheckEnvironment, kShutdownCheckEnvironmentValue, true) != 0) {
                std::_Exit(EXIT_FAILURE);
            }
            ::execl(kShutdownExecutablePath, kShutdownExecutablePath, static_cast<char*>(nullptr));
            std::_Exit(EXIT_FAILURE);
        },
        ::testing::ExitedWithCode(EXIT_SUCCESS),
        ""
    );
}
#endif

TEST(JsonValueBuilder, CustomArithmeticSerialization) {
    constexpr long double kInput = 1.25L;
    const auto expected = formats::serialize::Serialize(kInput, formats::serialize::To<formats::json::Value>{});
    ASSERT_TRUE(expected.IsObject());
    EXPECT_EQ(formats::json::ValueBuilder{kInput}.ExtractValue(), expected);
    EXPECT_EQ(formats::json::ValueBuilder{std::optional{kInput}}.ExtractValue(), expected);

    formats::json::ValueBuilder object;
    object["assigned"] = kInput;
    object.EmplaceNocheck("emplaced", kInput);
    const auto value = object.ExtractValue();
    EXPECT_EQ(value["assigned"], expected);
    EXPECT_EQ(value["emplaced"], expected);

    const std::map<std::string, long double> input{{"value", kInput}};
    EXPECT_EQ(formats::json::ValueBuilder{input}.ExtractValue()["value"], expected);
}

TEST(JsonValueBuilder, LongDoubleArrayInsertion) {
    constexpr long double kFirst = 1.25L;
    constexpr long double kSecond = -2.5L;
    const auto expected = formats::json::MakeArray(static_cast<double>(kFirst), static_cast<double>(kSecond));

    formats::json::ValueBuilder array{formats::json::Type::kArray};
    array.PushBack(kFirst);
    array.PushBack({kSecond});
    EXPECT_EQ(array.ExtractValue(), expected);
    const std::vector<long double> input{kFirst, kSecond};
    EXPECT_EQ(formats::json::ValueBuilder{input}.ExtractValue(), expected);
}

struct SerializableValue {
    std::function<formats::json::Value()> make_value;
    mutable std::size_t calls{0};
};

struct SerializableStringView {
    std::string_view value;

    operator std::string_view() const {
        ADD_FAILURE() << "Serialize must take priority over string conversion";
        return value;
    }
};

union SerializableStringViewUnion {
    std::string_view value;

    operator std::string_view() const {
        ADD_FAILURE() << "Serialize must take priority over string conversion";
        return value;
    }
};

formats::json::Value Serialize(const SerializableStringView& value, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder builder;
    builder.EmplaceNocheck("custom", value.value);
    return builder.ExtractValue();
}

formats::json::Value Serialize(const SerializableStringViewUnion& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::MakeObject("custom", value.value);
}

formats::json::Value Serialize(const SerializableValue& value, formats::serialize::To<formats::json::Value>) {
    ++value.calls;
    return value.make_value();
}

formats::json::Value Serialize(
    const std::optional<SerializableValue>& value,
    formats::serialize::To<formats::json::Value> to
) {
    return value ? Serialize(*value, to) : formats::json::Value{};
}

struct CustomWrappedValue {};

formats::json::Value
Serialize(const std::optional<CustomWrappedValue>& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::MakeObject("optional", value.has_value());
}

formats::json::Value
Serialize(const std::variant<CustomWrappedValue, int>& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::MakeObject("variant", value.index());
}

template <typename T>
void CheckSerializedInsertion(const T& input, const formats::json::Value& expected) {
    formats::json::ValueBuilder assigned;
    assigned = input;
    EXPECT_EQ(assigned.ExtractValue(), expected);

    formats::json::ValueBuilder array{formats::json::Type::kArray};
    array.PushBack(input);
    EXPECT_EQ(array.ExtractValue(), formats::json::MakeArray(expected));

    formats::json::ValueBuilder object{formats::json::Type::kObject};
    object.EmplaceNocheck("value", input);
    EXPECT_EQ(object.ExtractValue(), formats::json::MakeObject("value", expected));
}

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

template <typename Character>
void CheckCharacterInsertionCompatibility() {
    for (const auto character :
         {static_cast<Character>('A'), std::numeric_limits<Character>::min(), std::numeric_limits<Character>::max()})
    {
        const auto expected = static_cast<std::int64_t>(character);
        EXPECT_EQ(formats::json::ValueBuilder{character}.ExtractValue().As<std::int64_t>(), expected);
        EXPECT_EQ(
            Serialize(character, formats::serialize::To<formats::json::Value>{}).template As<std::int64_t>(),
            expected
        );

        formats::json::ValueBuilder object;
        object["assigned"] = character;
        object.EmplaceNocheck("emplaced", character);
        const auto value = object.ExtractValue();
        EXPECT_EQ(value["assigned"].As<std::int64_t>(), expected);
        EXPECT_EQ(value["emplaced"].As<std::int64_t>(), expected);

        formats::json::ValueBuilder array{formats::json::Type::kArray};
        array.PushBack(character);
        EXPECT_EQ(array.ExtractValue()[0].As<std::int64_t>(), expected);
    }
}

template <typename T>
void InsertValue(formats::json::ValueBuilder& builder, formats::json::Type type, T&& value) {
    if (type == formats::json::Type::kArray) {
        builder.PushBack(std::forward<T>(value));
    } else {
        builder.EmplaceNocheck("value", std::forward<T>(value));
    }
}

class JsonValueBuilderInsertion : public testing::TestWithParam<formats::json::Type> {
protected:
    template <typename T>
    void Insert(formats::json::ValueBuilder& builder, T&& value) const {
        InsertValue(builder, GetParam(), std::forward<T>(value));
    }
};

INSTANTIATE_TEST_SUITE_P(
    ArrayAndObject,
    JsonValueBuilderInsertion,
    testing::Values(formats::json::Type::kArray, formats::json::Type::kObject)
);

template <typename Float>
void TestInsertionNonFinite(Float value, formats::json::Type type) {
#ifdef NDEBUG
    formats::json::ValueBuilder builder(type);
    EXPECT_THROW(InsertValue(builder, type, value), formats::json::Exception);
#else
    const auto insert = [value, type] {
        formats::json::ValueBuilder builder(type);
        InsertValue(builder, type, value);
    };
    UEXPECT_DEATH(insert(), "");
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

TEST(JsonValueBuilder, CharacterInsertionPreservesExistingOverloads) {
    CheckCharacterInsertionCompatibility<char>();
    CheckCharacterInsertionCompatibility<wchar_t>();
    CheckCharacterInsertionCompatibility<char8_t>();
    CheckCharacterInsertionCompatibility<char16_t>();
    CheckCharacterInsertionCompatibility<char32_t>();
}

TEST(JsonValueBuilder, PushBackTemporaryCharacterBuilders) {
    formats::json::ValueBuilder builder{formats::json::Type::kArray};
    builder.PushBack(formats::json::ValueBuilder('a'));
    builder.PushBack(formats::json::ValueBuilder{'a'});

    const auto value = builder.ExtractValue();
    ASSERT_EQ(value.GetSize(), 2);
    EXPECT_EQ(value[0].As<int>(), static_cast<int>('a'));
    EXPECT_EQ(value[1].As<int>(), static_cast<int>('a'));
}

TEST(JsonValueBuilder, EmplaceNocheckTemporaryCharacterBuilders) {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    builder.EmplaceNocheck("parenthesized", formats::json::ValueBuilder('a'));
    builder.EmplaceNocheck("braced", formats::json::ValueBuilder{'a'});

    const auto value = builder.ExtractValue();
    ASSERT_EQ(value.GetSize(), 2);
    EXPECT_EQ(value["parenthesized"].As<int>(), static_cast<int>('a'));
    EXPECT_EQ(value["braced"].As<int>(), static_cast<int>('a'));
}

TEST(JsonValueBuilder, CharacterRangesPreserveNumericArraySerialization) {
    const std::vector<int> kExpectedCodeUnits{'A', 'B'};
    const auto check = [&kExpectedCodeUnits](const auto& characters) {
        const auto value = formats::serialize::Serialize(characters, formats::serialize::To<formats::json::Value>{});
        EXPECT_EQ(value.template As<std::vector<int>>(), kExpectedCodeUnits);
    };
    check(std::string{"AB"});
    check(std::string_view{"AB"});
    check(std::wstring{L"AB"});
    check(std::wstring_view{L"AB"});
    check(std::u8string{u8"AB"});
    check(std::u8string_view{u8"AB"});
    check(std::u16string{u"AB"});
    check(std::u16string_view{u"AB"});
    check(std::u32string{U"AB"});
    check(std::u32string_view{U"AB"});
}

TEST(JsonValueBuilder, CharacterContainersPreserveSerializationThroughBuilderOperations) {
    const auto expected = formats::json::MakeArray(static_cast<int>('A'), static_cast<int>('B'));
    const auto check = [&expected](const auto& characters) {
        EXPECT_EQ(formats::json::ValueBuilder{characters}.ExtractValue(), expected);

        formats::json::ValueBuilder object;
        object["assigned"] = characters;
        object.EmplaceNocheck("emplaced", characters);
        const auto object_value = object.ExtractValue();
        EXPECT_EQ(object_value["assigned"], expected);
        EXPECT_EQ(object_value["emplaced"], expected);

        formats::json::ValueBuilder array{formats::json::Type::kArray};
        array.PushBack(characters);
        EXPECT_EQ(array.ExtractValue()[0], expected);
    };
    check(std::vector<char>{'A', 'B'});
    check(std::vector<wchar_t>{L'A', L'B'});
    check(std::vector<char8_t>{u8'A', u8'B'});
    check(std::vector<char16_t>{u'A', u'B'});
    check(std::vector<char32_t>{U'A', U'B'});
}

TEST(JsonValueBuilder, DirectAssignmentPromotesArithmeticTypes) {
    formats::json::ValueBuilder builder;
    builder["signed-byte"] = static_cast<signed char>(-1);
    builder["unsigned-byte"] = static_cast<unsigned char>(2);
    builder["signed-short"] = static_cast<short>(-3);
    builder["unsigned-short"] = static_cast<unsigned short>(4);
    builder["signed-long-long"] = -5LL;
    builder["unsigned-long-long"] = 6ULL;
    builder["float"] = 8.5F;
    builder["signed-int"] = -9;
    builder["unsigned-int"] = 10U;
    builder["boolean"] = true;
    builder["null"] = nullptr;
    const auto value = builder.ExtractValue();
    EXPECT_EQ(value["signed-byte"].As<int>(), -1);
    EXPECT_EQ(value["unsigned-byte"].As<unsigned int>(), 2U);
    EXPECT_EQ(value["signed-short"].As<int>(), -3);
    EXPECT_EQ(value["unsigned-short"].As<unsigned int>(), 4U);
    EXPECT_EQ(value["signed-long-long"].As<std::int64_t>(), -5);
    EXPECT_EQ(value["unsigned-long-long"].As<std::uint64_t>(), 6);
    EXPECT_DOUBLE_EQ(value["float"].As<double>(), 8.5);
    EXPECT_EQ(value["signed-int"].As<int>(), -9);
    EXPECT_EQ(value["unsigned-int"].As<unsigned int>(), 10U);
    EXPECT_TRUE(value["boolean"].As<bool>());
    EXPECT_TRUE(value["null"].IsNull());
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

TEST_P(JsonValueBuilderInsertion, RvalueSources) {
    formats::json::ValueBuilder destination(GetParam());
    formats::json::ValueBuilder builder_source = 1;
    auto value_source = formats::json::ValueBuilder{2}.ExtractValue();

    Insert(destination, std::move(builder_source));
    Insert(destination, std::move(value_source));

    EXPECT_THAT(JsonElements(destination.ExtractValue()), testing::ElementsAre(JsonAs(1), JsonAs(2)));
}

TEST_P(JsonValueBuilderInsertion, WrongTypeRejectsRvalueSources) {
    formats::json::ValueBuilder destination = 0;
    auto source_value = formats::json::ValueBuilder{2}.ExtractValue();

    EXPECT_THROW(Insert(destination, formats::json::ValueBuilder{1}), formats::json::TypeMismatchException);
    EXPECT_THROW(Insert(destination, std::move(source_value)), formats::json::TypeMismatchException);

    EXPECT_EQ(destination.ExtractValue().As<int>(), 0);
    // source_value must remain unchanged on a type-mismatch exception.
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_EQ(source_value.As<int>(), 2);
}

TEST_P(JsonValueBuilderInsertion, ConcreteTypes) {
    formats::json::ValueBuilder builder(GetParam());
    char mutable_string[] = "mutable string";
    const std::string string = "string";

    Insert(builder, nullptr);
    Insert(builder, false);
    Insert(builder, "const char pointer");
    Insert(builder, mutable_string);
    Insert(builder, string);
    Insert(builder, std::string_view{"string view"});
    Insert(builder, 0);
    Insert(builder, 0u);
    Insert(builder, std::uint64_t{0});
    Insert(builder, std::int64_t{0});
    Insert(builder, 0.0f);
    Insert(builder, 0.0);

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

TEST_P(JsonValueBuilderInsertion, StringConvertibleInsertionPreservesCustomSerialization) {
    const auto check = [this](auto&& input) {
        formats::json::ValueBuilder builder(GetParam());
        Insert(builder, std::forward<decltype(input)>(input));
        const auto value = builder.ExtractValue();
        ASSERT_EQ(value.GetSize(), 1);
        EXPECT_EQ((*value.begin())["custom"].As<std::string>(), "text");
    };
    const SerializableStringView string_view{"text"};
    const SerializableStringViewUnion string_view_union{"text"};
    check(string_view);
    check(string_view_union);
    check(SerializableStringView{"text"});
    check(SerializableStringViewUnion{"text"});
}

TEST_P(JsonValueBuilderInsertion, BracedStringConvertibleInsertionPreservesCustomSerialization) {
    const auto check = [type = GetParam()](const auto& input) {
        formats::json::ValueBuilder builder(type);
        if (type == formats::json::Type::kArray) {
            builder.PushBack({input});
        } else {
            builder.EmplaceNocheck("value", {input});
        }
        const auto value = builder.ExtractValue();
        ASSERT_EQ(value.GetSize(), 1);
        EXPECT_EQ((*value.begin())["custom"].As<std::string>(), "text");
    };
    check(SerializableStringView{"text"});
    check(SerializableStringViewUnion{"text"});
}

TEST_P(JsonValueBuilderInsertion, PromotedArithmeticTypes) {
    formats::json::ValueBuilder builder(GetParam());

    Insert(builder, static_cast<signed char>(-1));
    Insert(builder, static_cast<unsigned char>(2));
    Insert(builder, static_cast<short>(-3));
    Insert(builder, static_cast<unsigned short>(4));
    Insert(builder, -5LL);
    Insert(builder, 6ULL);

    const auto value = builder.ExtractValue();
    EXPECT_THAT(
        JsonElements(value),
        testing::ElementsAre(
            JsonAs(-1),
            JsonAs(2u),
            JsonAs(-3),
            JsonAs(4u),
            JsonAs(std::int64_t{-5}),
            JsonAs(std::uint64_t{6})
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

TEST(JsonValueBuilder, PushBackStringKeepsElementBuilderValidAfterReallocation) {
    constexpr std::string_view kText = "string payload longer than the inline storage";
    constexpr std::string_view kReplacement = "replacement";
    formats::json::ValueBuilder builder(formats::json::Type::kArray);
    builder.Reserve(1);
    builder.PushBack(kText);
    auto first_element = builder[0];

    builder.PushBack(kText);
    first_element = kReplacement;

    const auto value = builder.ExtractValue();
    EXPECT_THAT(
        JsonElements(value),
        testing::ElementsAre(JsonAs(std::string{kReplacement}), JsonAs(std::string{kText}))
    );
}

TEST_P(JsonValueBuilderInsertion, StringInsertionOwnsBytesAfterSharedNullDetachment) {
    constexpr char kText[] = "string payload longer than inline storage\0with embedded null";
    const std::string expected{kText, sizeof(kText) - 1};
    auto text = expected;
    formats::json::ValueBuilder builder;
    const formats::json::ValueBuilder untouched_builder;
    const formats::json::Value untouched_value;

    Insert(builder, std::string_view{text});
    text.assign(text.size(), 'x');

    EXPECT_THAT(JsonElements(builder.ExtractValue()), testing::ElementsAre(JsonAs(expected)));
    EXPECT_TRUE(untouched_builder.IsNull());
    EXPECT_TRUE(untouched_value.IsNull());
}

TEST_P(JsonValueBuilderInsertion, RejectsNonFiniteNumbers) {
    TestInsertionNonFinite(std::numeric_limits<float>::quiet_NaN(), GetParam());
    TestInsertionNonFinite(std::numeric_limits<float>::infinity(), GetParam());
    TestInsertionNonFinite(std::numeric_limits<double>::quiet_NaN(), GetParam());
    TestInsertionNonFinite(std::numeric_limits<double>::infinity(), GetParam());
}

TEST_P(JsonValueBuilderInsertion, ValuePreservesBorrowedSource) {
    formats::json::ValueBuilder source(formats::common::Type::kObject);
    source["nested"]["value"] = 1;
    const auto source_value = source.ExtractValue();
    // Keep an owning alias to exercise copying from shared storage.
    const auto source_alias = source_value;  // NOLINT(performance-unnecessary-copy-initialization)

    formats::json::ValueBuilder result(GetParam());
    result.Reserve(1);
    Insert(result, source_value["nested"]);

    const auto value = result.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(IsJsonObjectWithValue(1)));
    EXPECT_EQ(source_value, source_alias);
    EXPECT_FALSE(value.begin()->DebugIsReferencingSameMemory(source_value["nested"]));
}

TEST_P(JsonValueBuilderInsertion, ValueMovesUniqueRootAndCopiesAlias) {
    formats::json::ValueBuilder unique_source(formats::common::Type::kObject);
    unique_source["value"] = 1;
    auto unique_value = unique_source.ExtractValue();

    formats::json::ValueBuilder result(GetParam());
    Insert(result, std::move(unique_value));
    EXPECT_TRUE(unique_value.IsNull());

    formats::json::ValueBuilder aliased_source(formats::common::Type::kObject);
    aliased_source["value"] = 2;
    auto aliased_value = aliased_source.ExtractValue();
    const auto alias = aliased_value;
    Insert(result, std::move(aliased_value));

    const auto value = result.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(IsJsonObjectWithValue(1), IsJsonObjectWithValue(2)));
    EXPECT_EQ(alias["value"].As<int>(), 2);
    EXPECT_FALSE(std::next(value.begin())->DebugIsReferencingSameMemory(alias));
}

TEST_P(JsonValueBuilderInsertion, ValuePreservesDuplicateMembers) {
    formats::json::ValueBuilder duplicate(formats::common::Type::kObject);
    duplicate.EmplaceNocheck("value", 1);
    duplicate.EmplaceNocheck("value", 2);
    const auto duplicate_value = duplicate.ExtractValue();

    formats::json::ValueBuilder result(GetParam());
    Insert(result, duplicate_value);

    const auto value = result.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(IsJsonObjectWithDuplicateValues(1, 2)));
}

TEST(JsonValueBuilder, EmplaceNocheckBracedValues) {
    formats::json::ValueBuilder builder(formats::common::Type::kObject);
    builder.EmplaceNocheck("value", {});
    builder.EmplaceNocheck("value", {0});
    builder.EmplaceNocheck("value", {0u});
    builder.EmplaceNocheck("value", {"const char pointer"});
    builder.EmplaceNocheck("value", {std::string{"foo"}});
    builder.EmplaceNocheck("value", {std::string_view{"string view"}});
    builder.EmplaceNocheck("value", {utils::StringLiteral{"string literal"}});
    builder.EmplaceNocheck("value", {formats::json::ValueBuilder{1}});

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

TEST(JsonValueBuilder, EmplaceNocheckRejectsMultipleBracedValues) {
    formats::json::ValueBuilder builder(formats::common::Type::kObject);

    EXPECT_THROW(builder.EmplaceNocheck("value", {1, 2}), formats::json::Exception);
    EXPECT_TRUE(builder.IsEmpty());
}

TEST(JsonValueBuilder, EmplaceNocheckArithmeticKeepsElementBuilderValidAfterReallocation) {
    formats::json::ValueBuilder builder(formats::common::Type::kObject);
    builder.Reserve(1);
    builder.EmplaceNocheck("value", 1);
    auto first_element = builder["value"];

    builder.EmplaceNocheck("second", 2);
    first_element = 3;

    const auto value = builder.ExtractValue();
    EXPECT_THAT(JsonElements(value), testing::ElementsAre(JsonAs(3), JsonAs(2)));
}

TEST(JsonValueBuilder, EmplaceNocheckCopiesStringsAndKeys) {
    formats::json::ValueBuilder builder;
    std::string key = "key";
    constexpr char kText[] = "copied string with embedded\0null";
    std::string text{kText, sizeof(kText) - 1};
    const auto expected = text;
    builder.EmplaceNocheck(key, std::string_view{text});
    key.assign("changed");
    text.assign("changed");
    const auto value = builder.ExtractValue();
    EXPECT_EQ(value["key"].As<std::string>(), expected);
}

TEST(JsonValueBuilder, EmplaceNocheckValidatesDestinationBeforeReadingValue) {
    formats::json::ValueBuilder destination{formats::json::Type::kArray};
    auto missing = formats::json::FromString("{}")["missing"];

    EXPECT_THROW(destination.EmplaceNocheck("value", missing), formats::json::TypeMismatchException);
    EXPECT_THROW(destination.EmplaceNocheck("value", std::move(missing)), formats::json::TypeMismatchException);
    EXPECT_TRUE(destination.IsArray());
    EXPECT_TRUE(destination.IsEmpty());
    EXPECT_TRUE(missing.IsMissing());
}

TEST(JsonValueBuilder, PushBackValidatesDestinationBeforeReadingValue) {
    for (const auto& original : {
             formats::json::FromString("42"),
             formats::json::MakeObject("existing", 42),
         })
    {
        formats::json::ValueBuilder destination{original};
        auto missing = formats::json::FromString("{}")["missing"];

        EXPECT_THROW(destination.PushBack(missing), formats::json::TypeMismatchException);
        EXPECT_THROW(destination.PushBack(std::move(missing)), formats::json::TypeMismatchException);
        EXPECT_EQ(destination.ExtractValue(), original);
        EXPECT_TRUE(missing.IsMissing());
    }
}

TEST(JsonValueBuilder, EmplaceNocheckPayloadExceptionPreservesDestination) {
    for (const auto& original : {formats::json::Value{}, formats::json::MakeObject("existing", 42)}) {
        for (const bool move_source : {false, true}) {
            SCOPED_TRACE(move_source);
            formats::json::ValueBuilder destination{original};
            auto missing = formats::json::FromString("{}")["missing"];

            if (move_source) {
                EXPECT_THROW(
                    destination.EmplaceNocheck("value", std::move(missing)),
                    formats::json::MemberMissingException
                );
            } else {
                EXPECT_THROW(destination.EmplaceNocheck("value", missing), formats::json::MemberMissingException);
            }
            EXPECT_EQ(destination.ExtractValue(), original);
            EXPECT_TRUE(missing.IsMissing());
        }
    }
}

TEST(JsonValueBuilder, EmplaceNocheckMovedValueAndKeySurviveGrowth) {
    constexpr std::string_view kKey = "key_owned_by_the_moved_json_value_beyond_inline_storage";
    std::string key{kKey};
    auto source = formats::json::MakeObject("key", key, "value", 42);
    formats::json::ValueBuilder destination{formats::json::Type::kObject};
    destination.Reserve(1);
    destination.EmplaceNocheck("existing", 1);
    auto existing = destination["existing"];

    destination.EmplaceNocheck(std::string_view{key}, std::move(source));
    key.assign(key.size(), 'x');
    EXPECT_TRUE(source.IsNull());
    existing = 2;

    const auto result = destination.ExtractValue();
    EXPECT_EQ(result["existing"].As<int>(), 2);
    EXPECT_EQ(result[kKey]["key"].As<std::string>(), kKey);
    EXPECT_EQ(result[kKey]["value"].As<int>(), 42);
}

TEST(JsonValueBuilder, EmplaceNocheckScalarsRejectWrongType) {
    formats::json::ValueBuilder builder{formats::json::Type::kArray};
    EXPECT_THROW(builder.EmplaceNocheck("value", nullptr), formats::json::TypeMismatchException);
    EXPECT_THROW(builder.EmplaceNocheck("value", true), formats::json::TypeMismatchException);
    EXPECT_THROW(builder.EmplaceNocheck("value", 1), formats::json::TypeMismatchException);
    EXPECT_THROW(builder.EmplaceNocheck("value", 1.5), formats::json::TypeMismatchException);
    EXPECT_THROW(builder.EmplaceNocheck("value", "text"), formats::json::TypeMismatchException);
    EXPECT_TRUE(builder.IsEmpty());
}

TEST(JsonValueBuilder, EmplaceNocheckCustomSerialization) {
    formats::json::ValueBuilder builder;
    builder.EmplaceNocheck("vector", std::vector<int>{1, 2});
    builder.EmplaceNocheck("optional", std::optional<int>{});
    const auto value = builder.ExtractValue();
    EXPECT_EQ(value["vector"].As<std::vector<int>>(), (std::vector<int>{1, 2}));
    EXPECT_TRUE(value["optional"].IsNull());
}

TEST(JsonValueBuilder, WrappedScalarInsertion) {
    CheckSerializedInsertion(std::optional<int>{42}, formats::json::ValueBuilder{42}.ExtractValue());
    CheckSerializedInsertion(std::optional<int>{}, formats::json::Value{});
    using Variant = std::variant<int, std::string>;
    CheckSerializedInsertion(Variant{42}, formats::json::ValueBuilder{42}.ExtractValue());
    CheckSerializedInsertion(Variant{std::string{"text"}}, formats::json::ValueBuilder{"text"}.ExtractValue());
}

TEST(JsonValueBuilder, WrappedInsertionUsesWrapperSerializer) {
    CheckSerializedInsertion(std::optional<CustomWrappedValue>{}, formats::json::MakeObject("optional", false));
    CheckSerializedInsertion(
        std::optional<CustomWrappedValue>{CustomWrappedValue{}},
        formats::json::MakeObject("optional", true)
    );
    using Variant = std::variant<CustomWrappedValue, int>;
    CheckSerializedInsertion(Variant{CustomWrappedValue{}}, formats::json::MakeObject("variant", 0));
    CheckSerializedInsertion(Variant{42}, formats::json::MakeObject("variant", 1));
}

TEST(JsonValueBuilder, SerializedInsertionPreservesSharedValues) {
    const auto source = formats::json::FromString(R"({"nested": {"value": 42}})");
    const auto original = source.Clone();
    for (const bool child : {false, true}) {
        const auto expected = child ? source["nested"] : source;
        const std::optional<SerializableValue> input{SerializableValue{[&expected] { return expected; }}};
        CheckSerializedInsertion(input, expected);
        EXPECT_EQ(input->calls, 3);
        EXPECT_EQ(source, original);
    }
    const std::optional<SerializableValue> unique_child{SerializableValue{[] {
        const auto parent = formats::json::MakeObject("nested", formats::json::MakeObject("value", 42));
        return parent["nested"];
    }}};
    CheckSerializedInsertion(unique_child, formats::json::MakeObject("value", 42));
    EXPECT_EQ(unique_child->calls, 3);
}

TEST(JsonValueBuilder, SerializedInsertionMissingValueLeavesDestinationUnchanged) {
    const auto source = formats::json::FromString(R"({"nested": {}})");
    const std::optional<SerializableValue> input{SerializableValue{[&source] { return source["nested"]["missing"]; }}};
    formats::json::ValueBuilder assigned{42};
    EXPECT_THROW(assigned = input, formats::json::MemberMissingException);
    EXPECT_EQ(assigned.ExtractValue().As<int>(), 42);

    formats::json::ValueBuilder array{formats::json::MakeArray(42)};
    EXPECT_THROW(array.PushBack(input), formats::json::MemberMissingException);
    EXPECT_EQ(array.ExtractValue(), formats::json::MakeArray(42));

    formats::json::ValueBuilder object{formats::json::MakeObject("existing", 42)};
    EXPECT_THROW(object.EmplaceNocheck("value", input), formats::json::MemberMissingException);
    EXPECT_EQ(object.ExtractValue(), formats::json::MakeObject("existing", 42));
    EXPECT_EQ(input->calls, 3);
}

TEST(JsonValueBuilder, DecimalSerializationUsesTranslationUnitOverloads) {
    const decimal64::Decimal<4> raw{"42"};
    const std::optional<decimal64::Decimal<4>> optional{decimal64::Decimal<4>{"32"}};
    constexpr std::int64_t kRawUnbiased = 420000;
    constexpr std::int64_t kOptionalUnbiased = 320000;

    EXPECT_EQ(
        formats::json::ValueBuilder{raw}.ExtractValue(),
        formats::json::ValueBuilder{kRawUnbiased}.ExtractValue()
    );

    formats::json::ValueBuilder custom;
    custom["raw"] = raw;
    custom["optional"] = optional;
    custom["empty"] = std::optional<decimal64::Decimal<4>>{};
    EXPECT_EQ(
        custom.ExtractValue(),
        formats::json::MakeObject("raw", kRawUnbiased, "optional", kOptionalUnbiased, "empty", nullptr)
    );

    formats::json::ValueBuilder array{formats::json::Type::kArray};
    array.PushBack(optional);
    EXPECT_EQ(array.ExtractValue(), formats::json::MakeArray(kOptionalUnbiased));

    formats::json::ValueBuilder object{formats::json::Type::kObject};
    object.EmplaceNocheck("optional", optional);
    EXPECT_EQ(object.ExtractValue(), formats::json::MakeObject("optional", kOptionalUnbiased));
}

TEST(JsonValueBuilder, SerializedFreshRoots) {
    const std::vector<formats::json::Value> sources{
        formats::json::FromString("null"),
        formats::json::FromString("42"),
        formats::json::FromString("[1, 2]"),
        formats::json::FromString(R"({"value": 42})")
    };
    for (const auto& source : sources) {
        const SerializableValue input{[&source] { return source.Clone(); }};
        formats::json::ValueBuilder result{input};
        EXPECT_EQ(input.calls, 1);
        EXPECT_EQ(result.GetPath(), formats::common::kPathRoot);
        EXPECT_EQ(result.ExtractValue(), source);
        EXPECT_TRUE(result.IsNull());
        result.EmplaceNocheck("reused", true);
        EXPECT_TRUE(result.ExtractValue()["reused"].As<bool>());
    }
}

TEST(JsonValueBuilder, SerializedSharedRootsAndChildrenAreCopied) {
    const auto source = formats::json::FromString(R"({"nested": {"value": 42}})");
    for (const bool child : {false, true}) {
        const SerializableValue input{[&source, child] { return child ? source["nested"] : source; }};
        formats::json::ValueBuilder result{input};
        if (child) {
            result["value"] = 100;
        } else {
            result["nested"]["value"] = 100;
        }
        EXPECT_EQ(input.calls, 1);
        EXPECT_EQ(result.GetPath(), formats::common::kPathRoot);
        EXPECT_EQ(source["nested"]["value"].As<int>(), 42);
    }
}

TEST(JsonValueBuilder, SerializedUniqueChildrenBecomeStorageRoots) {
    for (const bool drop_path : {false, true}) {
        const SerializableValue input{[drop_path] {
            auto source = formats::json::FromString(R"({"nested": {"value": 42}})");
            auto child = source["nested"];
            if (drop_path) {
                child.DropRootPath();
            }
            return child;
        }};
        formats::json::ValueBuilder result{input};
        EXPECT_EQ(input.calls, 1);
        EXPECT_EQ(result.GetPath(), formats::common::kPathRoot);
        EXPECT_EQ(result.GetSize(), 1);
        EXPECT_EQ(result["value"].GetPath(), "value");
        EXPECT_EQ(result.ExtractValue()["value"].As<int>(), 42);
    }
}

TEST(JsonValueBuilder, SerializedRootPreservesVersionAfterGrowth) {
    constexpr std::size_t kMembers = 128;
    const SerializableValue input{[] {
        formats::json::ValueBuilder source;
        auto first = source["first"];
        first = 42;
        for (std::size_t index = 0; index < kMembers; ++index) {
            source.EmplaceNocheck(std::to_string(index), index);
        }
        first = 43;
        return source.ExtractValue();
    }};
    formats::json::ValueBuilder result{input};
    auto first = result["first"];
    for (std::size_t index = kMembers; index < kMembers * 2; ++index) {
        result.EmplaceNocheck(std::to_string(index), index);
    }
    first = 44;
    EXPECT_EQ(input.calls, 1);
    const auto value = result.ExtractValue();
    EXPECT_EQ(value["first"].As<int>(), 44);
    constexpr auto kLastMember = kMembers * 2 - 1;
    EXPECT_EQ(value[std::to_string(kLastMember)].As<std::size_t>(), kLastMember);
}

TEST(JsonValueBuilder, SerializedMissingValuePreservesExceptionPath) {
    const auto source = formats::json::FromString(R"({"nested": {}})");
    const auto missing = source["nested"]["missing"];
    const SerializableValue input{[&missing] { return missing; }};
    std::string expected;
    try {
        const formats::json::ValueBuilder baseline{missing};
        FAIL() << "Missing value construction succeeded";
    } catch (const formats::json::MemberMissingException& exception) {
        expected = exception.what();
    }
    try {
        const formats::json::ValueBuilder result{input};
        FAIL() << "Serialized missing value construction succeeded";
    } catch (const formats::json::MemberMissingException& exception) {
        EXPECT_EQ(exception.what(), expected);
    }
    EXPECT_EQ(input.calls, 1);
}

TEST(JsonValueBuilder, SerializerExceptionIsPropagatedOnce) {
    const SerializableValue input{[]() -> formats::json::Value { throw std::runtime_error("serialize failed"); }};
    EXPECT_THROW(formats::json::ValueBuilder{input}, std::runtime_error);
    EXPECT_EQ(input.calls, 1);
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

TEST(JsonValueBuilder, AssignDescendantToAncestor) {
    auto builder = formats::json::FromString(R"({"child":{"value":42}})");
    formats::json::ValueBuilder root{builder};
    auto child = root["child"];
    root = child;
    EXPECT_EQ(root.ExtractValue()["value"].As<int>(), 42);
}

TEST(JsonValueBuilder, MoveAncestorIntoDescendant) {
    formats::json::ValueBuilder root{formats::json::FromString(R"({"value":42})")};
    auto child = root["child"];
    child = std::move(root);
    EXPECT_EQ(root.ExtractValue()["child"]["value"].As<int>(), 42);
}

TEST(JsonValueBuilder, DirectAssignmentPreservesSavedPaths) {
    formats::json::ValueBuilder root;
    auto leaf = root["nested"]["value"];
    root["nested"] = formats::json::FromString(R"({"value":1})");
    leaf = 42;
    EXPECT_EQ(root.ExtractValue()["nested"]["value"].As<int>(), 42);
}

TEST(JsonValueBuilder, MoveRootLeavesReusableNullBuilder) {
    formats::json::ValueBuilder source{42};
    formats::json::ValueBuilder destination{std::move(source)};
    EXPECT_TRUE(source.IsNull());
    source["value"] = 1;
    EXPECT_EQ(destination.ExtractValue().As<int>(), 42);
    EXPECT_EQ(source.ExtractValue()["value"].As<int>(), 1);
}

TEST(JsonValueBuilder, MoveValueRootLeavesReusableNullValue) {
    auto source = formats::json::FromString(R"({"value":42})");
    formats::json::ValueBuilder destination{std::move(source)};
    EXPECT_TRUE(source.IsNull());
    EXPECT_EQ(destination.ExtractValue()["value"].As<int>(), 42);
}

TEST(JsonValueBuilder, MoveSharedValuePreservesAliasesAndSource) {
    auto source = formats::json::FromString(R"({"value":42})");
    const auto alias = source;
    formats::json::ValueBuilder destination{std::move(source)};
    destination["value"] = 1;
    EXPECT_EQ(source["value"].As<int>(), 42);
    EXPECT_EQ(alias["value"].As<int>(), 42);
    EXPECT_EQ(destination.ExtractValue()["value"].As<int>(), 1);
}

TEST(JsonValueBuilder, SharedValueCopyPreservesStringViewBytes) {
    constexpr std::size_t kViewOffset = 1;
    constexpr std::size_t kStoragePadding = 2;
    for (const auto length : {8, 128, 1024}) {
        SCOPED_TRACE(length);
        std::string storage(length + kStoragePadding, 'x');
        storage[kViewOffset + length / 2] = '\0';
        const std::string_view view{storage.data() + kViewOffset, static_cast<std::size_t>(length)};
        const std::string expected{view};
        const auto source = formats::json::MakeArray(view);
        storage.assign(storage.size(), 'y');

        formats::json::ValueBuilder copied{source};
        auto shared = source;
        formats::json::ValueBuilder moved{std::move(shared)};

        EXPECT_EQ(source[0].As<std::string>(), expected);
        EXPECT_EQ(shared[0].As<std::string>(), expected);
        EXPECT_EQ(copied.ExtractValue()[0].As<std::string>(), expected);
        EXPECT_EQ(moved.ExtractValue()[0].As<std::string>(), expected);
        formats::json::ValueBuilder changed{std::move(shared)};
        changed[0] = "changed";
        EXPECT_EQ(changed.ExtractValue()[0].As<std::string>(), "changed");
        EXPECT_EQ(source[0].As<std::string>(), expected);
        EXPECT_EQ(shared[0].As<std::string>(), expected);
    }
}

TEST(JsonValueBuilder, MoveUniqueChildValuePreservesSourcePath) {
    auto source = formats::json::FromString(R"({"value":{"integer":42}})")["value"];
    formats::json::ValueBuilder destination{std::move(source)};
    EXPECT_TRUE(source.IsNull());
    EXPECT_EQ(source.GetPath(), "value");
    EXPECT_EQ(destination.ExtractValue()["integer"].As<int>(), 42);
}

TEST(JsonValueBuilder, PushBackSavedChildSurvivesArrayGrowth) {
    formats::json::ValueBuilder root{formats::json::FromString(R"([{"integer":42}])")};
    auto child = root[0];
    root.PushBack(std::move(child));
    const auto result = root.ExtractValue();
    EXPECT_EQ(result[0], result[1]);
    EXPECT_EQ(result[1]["integer"].As<int>(), 42);
}

TEST(JsonValueBuilder, MoveSharedRootInvalidatesSavedChildSafely) {
    formats::json::ValueBuilder source{formats::json::FromString(R"({"value":42})")};
    auto child = source["value"];
    formats::json::ValueBuilder destination{std::move(source)};
    EXPECT_TRUE(source.IsNull());
    EXPECT_THROW(child.GetSize(), formats::json::TypeMismatchException);
    EXPECT_EQ(destination.ExtractValue()["value"].As<int>(), 42);
}

TEST(JsonValueBuilder, DefaultNullBuildersHaveIndependentMutations) {
    formats::json::ValueBuilder first;
    formats::json::ValueBuilder second;
    first["value"] = 42;
    EXPECT_TRUE(second.IsNull());
    EXPECT_EQ(first.ExtractValue()["value"].As<int>(), 42);
    first.PushBack(1);
    EXPECT_EQ(first.ExtractValue()[0].As<int>(), 1);
    EXPECT_TRUE(second.IsNull());
}

TEST(JsonValueBuilder, DirectAssignmentAcceptsBracedValues) {
    formats::json::ValueBuilder builder;
    builder["empty"] = {};
    builder["integer"] = {42};
    builder["string"] = {"text"};
    const auto value = builder.ExtractValue();
    EXPECT_TRUE(value["empty"].IsNull());
    EXPECT_EQ(value["integer"].As<int>(), 42);
    EXPECT_EQ(value["string"].As<std::string>(), "text");
}

TEST(JsonValueBuilder, DirectAssignmentRejectsNonfiniteWithoutReplacingValue) {
    formats::json::ValueBuilder builder{42};
    UEXPECT_DEBUG_DEATH(
        EXPECT_THROW(builder = std::numeric_limits<double>::infinity(), formats::json::Exception),
        "Floating point"
    );
    UEXPECT_DEBUG_DEATH(
        EXPECT_THROW(builder = std::numeric_limits<float>::quiet_NaN(), formats::json::Exception),
        "Floating point"
    );
    EXPECT_EQ(builder.ExtractValue().As<int>(), 42);
}

TEST(JsonValueBuilder, DirectStringAssignmentOwnsEmbeddedNulls) {
    constexpr char kText[] = "first\0second";
    const std::string expected{kText, sizeof(kText) - 1};
    auto text = expected;
    formats::json::ValueBuilder root;
    root["value"] = std::string_view{text};
    text.assign(text.size(), 'x');
    EXPECT_EQ(root.ExtractValue()["value"].As<std::string>(), expected);
}

TEST(JsonValueBuilder, NativeArithmeticConstructionPreservesLimits) {
    const auto check = []<typename T>() {
        for (const auto input : {std::numeric_limits<T>::lowest(), T{}, std::numeric_limits<T>::max()}) {
            const auto value = formats::json::ValueBuilder{input}.ExtractValue();
            EXPECT_EQ(value.template As<T>(), input);
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
                const auto parsed = formats::json::FromString(std::to_string(input));
                EXPECT_EQ(value.IsInt(), parsed.IsInt());
                EXPECT_EQ(value.IsUInt(), parsed.IsUInt());
                EXPECT_EQ(value.IsInt64(), parsed.IsInt64());
                EXPECT_EQ(value.IsUInt64(), parsed.IsUInt64());
            }

            formats::json::ValueBuilder assigned;
            assigned = input;
            EXPECT_EQ(assigned.ExtractValue().template As<T>(), input);

            formats::json::ValueBuilder object{formats::json::Type::kObject};
            object.EmplaceNocheck("value", input);
            EXPECT_EQ(object.ExtractValue()["value"].template As<T>(), input);

            formats::json::ValueBuilder array{formats::json::Type::kArray};
            array.PushBack(input);
            EXPECT_EQ(array.ExtractValue()[0].template As<T>(), input);
        }
    };
    check.operator()<bool>();
    check.operator()<int>();
    check.operator()<unsigned int>();
    check.operator()<std::int64_t>();
    check.operator()<std::uint64_t>();
    check.operator()<signed char>();
    check.operator()<unsigned char>();
    check.operator()<short>();
    check.operator()<unsigned short>();
    check.operator()<long>();
    check.operator()<unsigned long>();
    check.operator()<long long>();
    check.operator()<unsigned long long>();
    check.operator()<float>();
    check.operator()<double>();
}

TEST(JsonValueBuilder, ArithmeticOperationsAcceptCvQualifiedInputs) {
    const auto check = []<typename T, typename Expected>(T& input, Expected expected) {
        EXPECT_EQ(formats::json::ValueBuilder{input}.ExtractValue().template As<Expected>(), expected);

        formats::json::ValueBuilder object;
        object["assigned"] = input;
        object.EmplaceNocheck("emplaced", input);
        object.EmplaceNocheck("braced", {input});
        const auto value = object.ExtractValue();
        EXPECT_EQ(value["assigned"].template As<Expected>(), expected);
        EXPECT_EQ(value["emplaced"].template As<Expected>(), expected);
        EXPECT_EQ(value["braced"].template As<Expected>(), expected);

        formats::json::ValueBuilder array{formats::json::Type::kArray};
        array.PushBack(input);
        array.PushBack({input});
        const auto elements = array.ExtractValue();
        EXPECT_EQ(elements[0].template As<Expected>(), expected);
        EXPECT_EQ(elements[1].template As<Expected>(), expected);
    };
    const volatile short integer = -42;
    volatile unsigned long long unsigned_integer = std::numeric_limits<unsigned long long>::max();
    const volatile char8_t character = u8'A';
    const volatile double floating = 1.25;
    volatile float single_precision = -2.5F;
    const volatile bool boolean = true;
    check(integer, -42);
    check(unsigned_integer, std::numeric_limits<std::uint64_t>::max());
    check(character, static_cast<int>(u8'A'));
    check(floating, 1.25);
    check(single_precision, -2.5F);
    check(boolean, true);
}

TEST(JsonValueBuilder, ArithmeticOperationsAcceptExplicitCvQualifiedTemplateArguments) {
    using Input = const volatile short;
    Input input = -42;
    formats::json::ValueBuilder assigned;
    assigned.operator= <Input>(input);
    EXPECT_EQ(assigned.ExtractValue().As<int>(), -42);

    formats::json::ValueBuilder object;
    object.EmplaceNocheck<Input>("value", input);
    EXPECT_EQ(object.ExtractValue()["value"].As<int>(), -42);

    formats::json::ValueBuilder array{formats::json::Type::kArray};
    array.PushBack<Input>(input);
    EXPECT_EQ(array.ExtractValue()[0].As<int>(), -42);
}

TEST(JsonValueBuilder, CharacterOperationsAcceptExplicitCvQualifiedTemplateArguments) {
    const auto check = []<typename Character>() {
        using Input = const volatile Character;
        Input input = static_cast<Character>('A');
        const auto expected = static_cast<std::int64_t>(input);

        formats::json::ValueBuilder assigned;
        EXPECT_EQ(&assigned.operator= <Input>(input), &assigned);
        EXPECT_EQ(assigned.ExtractValue().As<std::int64_t>(), expected);

        formats::json::ValueBuilder object;
        object.EmplaceNocheck<Input>("value", input);
        EXPECT_EQ(object.ExtractValue()["value"].As<std::int64_t>(), expected);

        formats::json::ValueBuilder array{formats::json::Type::kArray};
        array.PushBack<Input>(input);
        EXPECT_EQ(array.ExtractValue()[0].As<std::int64_t>(), expected);
    };
    check.operator()<char>();
    check.operator()<wchar_t>();
    check.operator()<char8_t>();
    check.operator()<char16_t>();
    check.operator()<char32_t>();
}

TEST(Meta, IsNativeArithmetic) {
    static_assert(formats::json::impl::IsNativeArithmetic<bool>);
    static_assert(formats::json::impl::IsNativeArithmetic<int>);
    static_assert(formats::json::impl::IsNativeArithmetic<unsigned int>);
    static_assert(formats::json::impl::IsNativeArithmetic<std::int64_t>);
    static_assert(formats::json::impl::IsNativeArithmetic<std::uint64_t>);
    static_assert(formats::json::impl::IsNativeArithmetic<float>);
    static_assert(formats::json::impl::IsNativeArithmetic<double>);
    static_assert(formats::json::impl::IsNativeArithmetic<signed char>);
    static_assert(formats::json::impl::IsNativeArithmetic<unsigned char>);
    static_assert(formats::json::impl::IsNativeArithmetic<short>);
    static_assert(formats::json::impl::IsNativeArithmetic<unsigned short>);
    static_assert(formats::json::impl::IsNativeArithmetic<char>);
    static_assert(formats::json::impl::IsNativeArithmetic<wchar_t>);
    static_assert(formats::json::impl::IsNativeArithmetic<char8_t>);
    static_assert(formats::json::impl::IsNativeArithmetic<char16_t>);
    static_assert(formats::json::impl::IsNativeArithmetic<char32_t>);
    static_assert(std::is_constructible_v<formats::json::ValueBuilder, const int>);
    static_assert(std::is_constructible_v<formats::json::ValueBuilder, volatile double>);
    static_assert(std::is_constructible_v<formats::json::ValueBuilder, volatile float&>);
    static_assert(std::is_constructible_v<formats::json::ValueBuilder, const volatile double&>);
    static_assert(std::is_constructible_v<formats::json::ValueBuilder, const volatile char8_t>);

    static_assert(!formats::json::impl::IsNativeArithmetic<long double>);
    static_assert(!formats::json::impl::IsNativeArithmetic<int&>);
    static_assert(!formats::json::impl::IsNativeArithmetic<void>);
    static_assert(!formats::json::impl::IsNativeArithmetic<std::string>);
}

TEST(Meta, IsNativeString) {
    struct Convertible {
        operator std::string_view() const { return "text"; }
    };
    struct ExplicitConvertible {
        explicit operator std::string_view() const { return "text"; }
    };
    struct MutableConvertible {
        operator std::string_view() { return "text"; }
    };

    static_assert(formats::json::impl::IsNativeString<std::string>);
    static_assert(formats::json::impl::IsNativeString<std::string_view>);
    static_assert(formats::json::impl::IsNativeString<char*>);
    static_assert(formats::json::impl::IsNativeString<const char*>);
    static_assert(formats::json::impl::IsNativeString<char[5]>);
    static_assert(formats::json::impl::IsNativeString<const char[5]>);
    static_assert(std::is_constructible_v<formats::json::ValueBuilder, const std::string&>);
    static_assert(formats::json::impl::IsNativeString<Convertible>);

    static_assert(!formats::json::impl::IsNativeString<SerializableStringView>);
    static_assert(!formats::json::impl::IsNativeString<ExplicitConvertible>);
    static_assert(!formats::json::impl::IsNativeString<MutableConvertible>);
    static_assert(!formats::json::impl::IsNativeString<std::wstring>);
    static_assert(!formats::json::impl::IsNativeString<std::u8string>);
    static_assert(!formats::json::impl::IsNativeString<char>);
    static_assert(!formats::json::impl::IsNativeString<int>);
    static_assert(!formats::json::impl::IsNativeString<std::vector<char>>);
}

TEST(JsonValueBuilder, StringConstructionCopiesPointerAndConvertibleInputs) {
    char text[] = "text";
    char* mutable_pointer = text;
    const char* const_pointer = text;
    const auto from_array = formats::json::ValueBuilder{text}.ExtractValue();
    const auto from_mutable_pointer = formats::json::ValueBuilder{mutable_pointer}.ExtractValue();
    const auto from_const_pointer = formats::json::ValueBuilder{const_pointer}.ExtractValue();
    const auto from_convertible = formats::json::ValueBuilder{utils::StringLiteral{"text"}}.ExtractValue();

    text[0] = 'n';

    EXPECT_EQ(from_array.As<std::string>(), "text");
    EXPECT_EQ(from_mutable_pointer.As<std::string>(), "text");
    EXPECT_EQ(from_const_pointer.As<std::string>(), "text");
    EXPECT_EQ(from_convertible.As<std::string>(), "text");
}

TEST(JsonValueBuilder, NativeArithmeticConstructionRejectsNonfinite) {
    UEXPECT_DEBUG_DEATH(
        EXPECT_THROW(formats::json::ValueBuilder{std::numeric_limits<float>::quiet_NaN()}, formats::json::Exception),
        "Floating point"
    );
    UEXPECT_DEBUG_DEATH(
        EXPECT_THROW(formats::json::ValueBuilder{std::numeric_limits<double>::infinity()}, formats::json::Exception),
        "Floating point"
    );
}

TEST(JsonValueBuilder, StringConstructionOwnsEmbeddedNulls) {
    constexpr char kText[] = "string payload beyond inline storage\0with embedded null";
    const std::string expected{kText, sizeof(kText) - 1};
    auto text = expected;
    const auto from_string = formats::json::ValueBuilder{text}.ExtractValue();
    const auto from_view = formats::json::ValueBuilder{std::string_view{text}}.ExtractValue();

    text.assign(text.size(), 'x');

    EXPECT_EQ(from_string.As<std::string>(), expected);
    EXPECT_EQ(from_view.As<std::string>(), expected);
}

TEST(JsonValueBuilder, StringConvertibleAssignmentPreservesCustomSerialization) {
    const auto check = [](auto&& input) {
        const auto expected = Serialize(input, formats::serialize::To<formats::json::Value>{});
        const auto constructed = formats::json::ValueBuilder{std::forward<decltype(input)>(input)}.ExtractValue();
        EXPECT_EQ(constructed, expected);
        formats::json::ValueBuilder builder;
        builder["value"] = std::forward<decltype(input)>(input);
        EXPECT_EQ(builder.ExtractValue()["value"], expected);
    };
    check(SerializableStringView{"text"});
    check(SerializableStringViewUnion{"text"});
}

TEST(JsonValueBuilder, NullSelfAppendPreservesNullSnapshot) {
    formats::json::ValueBuilder builder;
    builder.PushBack(std::move(builder));
    EXPECT_EQ(builder.ExtractValue(), formats::json::MakeArray(nullptr));
}

TEST(JsonValueBuilder, NestedChildrenSurvivePathPromotionAndParentDestruction) {
    constexpr std::size_t kExtraMembers = 128;
    formats::json::ValueBuilder root{formats::json::Type::kObject};
    std::optional<formats::json::ValueBuilder> first;
    std::optional<formats::json::ValueBuilder> second;
    {
        auto parent = root["nested"];
        first.emplace(formats::common::TransferTag{}, parent["first"]);
        second.emplace(formats::common::TransferTag{}, parent["second"]);
        for (std::size_t index = 0; index < kExtraMembers; ++index) {
            parent.EmplaceNocheck(std::to_string(index), index);
        }
    }
    for (std::size_t index = 0; index < kExtraMembers; ++index) {
        root.EmplaceNocheck(std::to_string(index), index);
    }
    *first = 42;
    *second = "value";
    EXPECT_EQ(first->GetPath(), "nested.first");
    EXPECT_EQ(second->GetPath(), "nested.second");
    const auto result = root.ExtractValue();
    EXPECT_EQ(result["nested"]["first"].As<int>(), 42);
    EXPECT_EQ(result["nested"]["second"].As<std::string>(), "value");
}

TEST(JsonValueBuilder, MovingPromotedChildAfterRootDestructionExtractsChild) {
    constexpr int kValue = 42;
    std::optional<formats::json::ValueBuilder> child;
    {
        formats::json::ValueBuilder root;
        child.emplace(formats::common::TransferTag{}, root["nested"]);
        (*child)["value"] = kValue;
    }
    formats::json::ValueBuilder moved{std::move(*child)};
    EXPECT_EQ(moved.ExtractValue(), formats::json::MakeObject("value", kValue));
}

TEST(JsonValueBuilder, NullChildSelfAppendPreservesNullSnapshot) {
    formats::json::ValueBuilder builder;
    auto child = builder["child"];
    child.PushBack(std::move(child));
    EXPECT_EQ(builder.ExtractValue(), formats::json::MakeObject("child", formats::json::MakeArray(nullptr)));
}

TEST(JsonValueBuilder, InvalidPushBackPreservesMoveSources) {
    formats::json::ValueBuilder destination{42};
    formats::json::ValueBuilder builder{formats::json::FromString(R"({"value":1})")};
    auto value = formats::json::FromString(R"({"value":2})");
    EXPECT_THROW(destination.PushBack(std::move(builder)), formats::json::TypeMismatchException);
    EXPECT_THROW(destination.PushBack(std::move(value)), formats::json::TypeMismatchException);
    EXPECT_EQ(builder.ExtractValue()["value"].As<int>(), 1);
    EXPECT_EQ(value["value"].As<int>(), 2);
    EXPECT_EQ(destination.ExtractValue().As<int>(), 42);
}

USERVER_NAMESPACE_END
