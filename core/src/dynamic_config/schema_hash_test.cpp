#include <userver/utest/utest.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gmock/gmock.h>

#include <dynamic_config/variables/USERVER_CACHES.hpp>

#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/registered_config_meta.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ::testing::AllOf;
using ::testing::Contains;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

using RegisteredConfigMeta = dynamic_config::RegisteredConfigMeta;

constexpr dynamic_config::SchemaHash kBooleanSchemaHash{"opaque-boolean-schema-hash"};
constexpr dynamic_config::SchemaHash kIntegerSchemaHash{"opaque-integer-schema-hash"};
constexpr dynamic_config::SchemaHash kObjectSchemaHash{"opaque-object-schema-hash"};
constexpr dynamic_config::SchemaHash kArraySchemaHash{"opaque-array-schema-hash"};
constexpr dynamic_config::SchemaHash kNullSchemaHash{"opaque-null-schema-hash"};
constexpr dynamic_config::SchemaHash kDuplicateFirstSchemaHash{"opaque-duplicate-first-schema-hash"};
constexpr dynamic_config::SchemaHash kDuplicateSecondSchemaHash{"opaque-duplicate-second-schema-hash"};

static_assert(dynamic_config::SchemaHash{}.value.empty());
static_assert(kBooleanSchemaHash.value == "opaque-boolean-schema-hash");
static_assert(std::is_default_constructible_v<dynamic_config::SchemaHash>);
static_assert(std::is_constructible_v<dynamic_config::SchemaHash, std::string_view>);
static_assert(!std::is_convertible_v<std::string_view, dynamic_config::SchemaHash>);
static_assert(!std::is_aggregate_v<dynamic_config::SchemaHash>);
static_assert(std::is_constructible_v<dynamic_config::Key<bool>, std::string_view, bool>);
static_assert(std::is_constructible_v<dynamic_config::ConfigDefault, std::string_view, bool>);

constexpr std::string_view kStructConfigName = "SCHEMA_HASH_STRUCT_CONFIG";
constexpr std::string_view kTypedBooleanConfigName = "SCHEMA_HASH_TYPED_BOOLEAN_CONFIG";
constexpr std::string_view kCompositeIntegerConfigName = "SCHEMA_HASH_COMPOSITE_INTEGER";
constexpr std::string_view kCompositeFlagName = "SCHEMA_HASH_COMPOSITE_FLAG";
constexpr std::string_view kSingleItemCompositeName = "SCHEMA_HASH_SINGLE_ITEM_COMPOSITE";
constexpr std::string_view kScalarDefaultName = "SCHEMA_HASH_DEFAULT_SCALAR";
constexpr std::string_view kArrayDefaultName = "SCHEMA_HASH_DEFAULT_ARRAY";
constexpr std::string_view kNullDefaultName = "SCHEMA_HASH_DEFAULT_NULL";
constexpr std::string_view kObjectDefaultName = "SCHEMA_HASH_DEFAULT_OBJECT";
constexpr std::string_view kDuplicateDefaultName = "SCHEMA_HASH_DUPLICATE_EQUAL_DEFAULT";

struct StructConfig final {
    bool enabled;
    std::chrono::milliseconds period;
};

StructConfig Parse(const formats::json::Value& value, formats::parse::To<StructConfig>) {
    return {
        .enabled = value["enabled"].As<bool>(),
        .period = value["period-ms"].As<std::chrono::milliseconds>(),
    };
}

const dynamic_config::Key<StructConfig> kStructConfig{
    kStructConfigName,
    dynamic_config::DefaultAsJsonString{R"({"enabled":false,"period-ms":17000})"},
    kObjectSchemaHash,
};

const dynamic_config::Key<bool> kTypedBooleanConfig{kTypedBooleanConfigName, false, kBooleanSchemaHash};

const dynamic_config::Key<StructConfig> kInternalDerivedConfig{
    dynamic_config::impl::InternalTag{},
    kStructConfigName,
};

int ParseCompositeConfig(const dynamic_config::DocsMap& docs_map) {
    return docs_map.Get(kCompositeIntegerConfigName).As<int>();
}

const dynamic_config::ConfigDefault kCompositeDefaults[]{
    {kCompositeIntegerConfigName, dynamic_config::DefaultAsJsonString{"42"}, kIntegerSchemaHash},
    {kCompositeFlagName, dynamic_config::DefaultAsJsonString{"false"}, kBooleanSchemaHash},
};

const dynamic_config::Key<int> kCompositeConfig{ParseCompositeConfig, kCompositeDefaults};

int ParseSingleItemComposite(const dynamic_config::DocsMap& docs_map) {
    return docs_map.Get(kSingleItemCompositeName).As<int>();
}

const dynamic_config::ConfigDefault kSingleItemCompositeDefaults[]{
    {kSingleItemCompositeName, dynamic_config::DefaultAsJsonString{"17"}, kIntegerSchemaHash},
};

const dynamic_config::Key<int> kSingleItemComposite{
    ParseSingleItemComposite,
    kSingleItemCompositeDefaults,
};

bool ParseHeterogeneousDefaults(const dynamic_config::DocsMap& docs_map) {
    const auto array = docs_map.Get(kArrayDefaultName);
    return docs_map.Get(kScalarDefaultName).As<int>() == 17 && array.IsArray() && array.GetSize() == 2 &&
           array[0].As<int>() == 1 && array[1].As<int>() == 2 && docs_map.Get(kNullDefaultName).IsNull() &&
           docs_map.Get(kObjectDefaultName)["enabled"].As<bool>();
}

const dynamic_config::ConfigDefault kHeterogeneousDefaults[]{
    {kScalarDefaultName, dynamic_config::DefaultAsJsonString{"17"}, kIntegerSchemaHash},
    {kArrayDefaultName, dynamic_config::DefaultAsJsonString{"[1, 2]"}, kArraySchemaHash},
    {kNullDefaultName, dynamic_config::DefaultAsJsonString{"null"}, kNullSchemaHash},
    {kObjectDefaultName, dynamic_config::DefaultAsJsonString{R"({"enabled": true})"}, kObjectSchemaHash},
};

const dynamic_config::Key<bool> kHeterogeneousDefaultsConfig{
    ParseHeterogeneousDefaults,
    kHeterogeneousDefaults,
};

int ParseDuplicateDefault(const dynamic_config::DocsMap& docs_map) {
    return docs_map.Get(kDuplicateDefaultName).As<int>();
}

const dynamic_config::ConfigDefault kDuplicateEqualDefaults[]{
    {kDuplicateDefaultName, dynamic_config::DefaultAsJsonString{"42"}, kDuplicateFirstSchemaHash},
    {kDuplicateDefaultName, dynamic_config::DefaultAsJsonString{" 42 "}, kDuplicateSecondSchemaHash},
};

const dynamic_config::Key<int> kDuplicateEqualDefaultConfig{
    ParseDuplicateDefault,
    kDuplicateEqualDefaults,
};

auto HasMetadata(std::string_view name, dynamic_config::SchemaHash schema_hash, std::string_view default_json) {
    return AllOf(
        Field(&RegisteredConfigMeta::name, name),
        Field(&RegisteredConfigMeta::schema_hash, schema_hash.value),
        Field(&RegisteredConfigMeta::default_as_json_string, default_json)
    );
}

UTEST(DynamicConfig, SchemaHashTypeContract) {
    EXPECT_THAT(dynamic_config::SchemaHash{}.value, IsEmpty());
    EXPECT_EQ(kBooleanSchemaHash.value, "opaque-boolean-schema-hash");
}

UTEST(DynamicConfig, RegisteredMetadataForTypedAndJsonKeys) {
    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();

    EXPECT_THAT(registered_configs, Contains(HasMetadata(kTypedBooleanConfigName, kBooleanSchemaHash, "false")));
    EXPECT_THAT(
        registered_configs,
        Contains(HasMetadata(kStructConfigName, kObjectSchemaHash, R"({"enabled":false,"period-ms":17000})"))
    );
}

UTEST(DynamicConfig, RegisteredMetadataMatchesGeneratedSchemaHash) {
    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    const auto generated_hash = ::dynamic_config::userver_caches::GetSchemaHash().value;

    EXPECT_THAT(generated_hash, Not(IsEmpty()));
    EXPECT_THAT(
        registered_configs,
        Contains(AllOf(
            Field(&RegisteredConfigMeta::name, "USERVER_CACHES"),
            Field(&RegisteredConfigMeta::schema_hash, generated_hash)
        ))
    );
}

UTEST(DynamicConfig, InternalDerivedKeyIsNotReportedInRegisteredConfigsMeta) {
    EXPECT_EQ(kInternalDerivedConfig.GetName(), kStructConfigName);
    const auto& config = dynamic_config::GetDefaultSnapshot()[kInternalDerivedConfig];
    EXPECT_FALSE(config.enabled);
    EXPECT_EQ(config.period, std::chrono::seconds{17});

    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    auto matching_configs =
        registered_configs |
        std::views::filter([](const auto& metadata) { return metadata.name == kStructConfigName; });
    const auto metadata = utils::AsContainer<std::vector<RegisteredConfigMeta>>(matching_configs);

    EXPECT_THAT(
        metadata,
        ElementsAre(HasMetadata(kStructConfigName, kObjectSchemaHash, R"({"enabled":false,"period-ms":17000})"))
    );
}

UTEST(DynamicConfig, CompositeKeyPreservesSchemaHashesAndDefaults) {
    EXPECT_EQ(dynamic_config::GetDefaultSnapshot()[kCompositeConfig], 42);

    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    EXPECT_THAT(registered_configs, Contains(HasMetadata(kCompositeIntegerConfigName, kIntegerSchemaHash, "42")));
    EXPECT_THAT(registered_configs, Contains(HasMetadata(kCompositeFlagName, kBooleanSchemaHash, "false")));
}

UTEST(DynamicConfig, ConfigDefaultOwnsSchemaHash) {
    constexpr std::string_view kOriginalSchemaHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    std::string schema_hash{kOriginalSchemaHash};
    const dynamic_config::ConfigDefault json_default{
        "OWNED_JSON_SCHEMA_HASH",
        dynamic_config::DefaultAsJsonString{"false"},
        dynamic_config::SchemaHash{schema_hash},
    };

    std::ranges::fill(schema_hash, 'x');

    EXPECT_EQ(json_default.schema_hash, kOriginalSchemaHash);
}

UTEST(DynamicConfig, TypedConfigDefaultUsesLegacyNoHashOverload) {
    const dynamic_config::ConfigDefault typed_default{"LEGACY_TYPED_DEFAULT", false};

    EXPECT_EQ(typed_default.default_json, "false");
    EXPECT_THAT(typed_default.schema_hash, IsEmpty());
}

UTEST(DynamicConfig, RegisteredConfigMetaSupportsLegacyTwoFieldInitialization) {
    const dynamic_config::RegisteredConfigMeta metadata{"LEGACY_CONFIG", "legacy-schema-hash"};

    EXPECT_EQ(metadata.name, "LEGACY_CONFIG");
    EXPECT_EQ(metadata.schema_hash, "legacy-schema-hash");
    EXPECT_THAT(metadata.default_as_json_string, IsEmpty());
}

UTEST(DynamicConfig, SingleItemCompositeHasNameAndDefault) {
    EXPECT_EQ(kSingleItemComposite.GetName(), kSingleItemCompositeName);
    EXPECT_EQ(dynamic_config::GetDefaultSnapshot()[kSingleItemComposite], 17);
}

UTEST(DynamicConfig, DefaultsSupportEveryJsonKind) {
    const auto defaults = dynamic_config::impl::MakeDefaultDocsMap();
    const auto array = defaults.Get(kArrayDefaultName);

    EXPECT_EQ(defaults.Get(kScalarDefaultName).As<int>(), 17);
    ASSERT_TRUE(array.IsArray());
    ASSERT_EQ(array.GetSize(), 2);
    EXPECT_EQ(array[0].As<int>(), 1);
    EXPECT_EQ(array[1].As<int>(), 2);
    EXPECT_TRUE(defaults.Get(kNullDefaultName).IsNull());
    EXPECT_TRUE(defaults.Get(kObjectDefaultName)["enabled"].As<bool>());
    EXPECT_TRUE(dynamic_config::GetDefaultSnapshot()[kHeterogeneousDefaultsConfig]);

    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    EXPECT_THAT(registered_configs, Contains(HasMetadata(kScalarDefaultName, kIntegerSchemaHash, "17")));
    EXPECT_THAT(registered_configs, Contains(HasMetadata(kArrayDefaultName, kArraySchemaHash, "[1, 2]")));
    EXPECT_THAT(registered_configs, Contains(HasMetadata(kNullDefaultName, kNullSchemaHash, "null")));
    EXPECT_THAT(
        registered_configs,
        Contains(HasMetadata(kObjectDefaultName, kObjectSchemaHash, R"({"enabled": true})"))
    );
}

UTEST(DynamicConfig, DefaultJsonErrorPathContainsConfigName) {
    const auto defaults = dynamic_config::impl::MakeDefaultDocsMap();
    UEXPECT_THROW_MSG(defaults.Get(kScalarDefaultName).As<std::string>(), std::exception, "SCHEMA_HASH_DEFAULT_SCALAR");
}

UTEST(DynamicConfig, DuplicateEqualDefaultsPreserveMetadata) {
    EXPECT_EQ(dynamic_config::GetDefaultSnapshot()[kDuplicateEqualDefaultConfig], 42);

    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    auto matching_configs =
        registered_configs |
        std::views::filter([](const auto& metadata) { return metadata.name == kDuplicateDefaultName; });
    const auto metadata = utils::AsContainer<std::vector<RegisteredConfigMeta>>(matching_configs);

    EXPECT_THAT(
        metadata,
        UnorderedElementsAre(
            HasMetadata(kDuplicateDefaultName, kDuplicateFirstSchemaHash, "42"),
            HasMetadata(kDuplicateDefaultName, kDuplicateSecondSchemaHash, " 42 ")
        )
    );
}

UTEST(DynamicConfig, OnlyToyConfigsHaveNoSchemaHash) {
    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    EXPECT_THAT(registered_configs, Each(Field(&RegisteredConfigMeta::name, Not(IsEmpty()))));

    auto hashless_config_names_view =
        registered_configs | std::views::filter([](const auto& metadata) { return metadata.schema_hash.empty(); }) |
        std::views::transform([](const auto& metadata) { return metadata.name; });
    const auto hashless_config_names = utils::AsContainer<std::vector<std::string_view>>(hashless_config_names_view);

    EXPECT_THAT(
        hashless_config_names,
        UnorderedElementsAre(
            "SAMPLE_STRUCT_CONFIG",
            "SAMPLE_BOOL_CONFIG",
            "DOC_MY_CONFIG",
            "SAMPLE_INTEGER_FROM_RUNTIME_CONFIG",
            "DYNAMIC_CONFIG_UPDATES_SINK_CHAIN",
            "USERVER_TEST_LIGHT_CLIENT_INT_CONFIG"
        )
    );
}

}  // namespace

USERVER_NAMESPACE_END
