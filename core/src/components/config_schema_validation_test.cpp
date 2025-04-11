#include <cache/lru_cache_component_base_test.hpp>

#include <components/component_list_test.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void ValidateExampleCacheConfig(const formats::yaml::Value& static_config) {
    yaml_config::impl::Validate(
        yaml_config::YamlConfig(static_config["example-cache"], {}), ExampleCacheComponent::GetStaticConfigSchema()
    );
}

}  // namespace

TEST(StaticConfigValidator, ValidConfig) {
    const std::string kStaticConfig = R"(
        example-cache:
            size: 1
            ways: 1
            lifetime: 1s # 0 (unlimited) by default
            config-settings: false # true by default
    )";
    ValidateExampleCacheConfig(formats::yaml::FromString(std::string{kStaticConfig}));
}

TEST(StaticConfigValidator, InvalidFieldName) {
    const std::string kInvalidStaticConfig = R"(
        example-cache:
            size: 1
            ways: 1
            not_declared_property: 1
    )";
    UEXPECT_THROW_MSG(
        ValidateExampleCacheConfig(formats::yaml::FromString(kInvalidStaticConfig)),
        std::runtime_error,
        "Error while validating static config against schema. Field "
        "'example-cache.not_declared_property' is not declared in schema '/'"
    );
}

TEST(StaticConfigValidator, InvalidFieldType) {
    const std::string kInvalidStaticConfig = R"(
        example-cache:
            size: 1
            ways: abc # must be integer
    )";
    UEXPECT_THROW_MSG(
        ValidateExampleCacheConfig(formats::yaml::FromString(kInvalidStaticConfig)),
        std::runtime_error,
        "Error while validating static config against schema. Value "
        "'abc' of field 'example-cache.ways' must be integer"
    );
}

namespace {

class TestComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "test-component";

    TestComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        [[maybe_unused]] const auto some_config_option = config["some-config-option"].As<int>();
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
            type: object
            description: config of InvalidSchemaTestComponent
            additionalProperties: false
            properties:
                some-config-option:
                    type: integer
                    description: some-config-option description
        )");
    }
};

constexpr std::string_view kTestComponentConfigPatch = R"(
components_manager:
    components:
        test-component:
            some-config-option: 42
)";

using StaticConfigValidatorTest = ComponentList;

}  // namespace

TEST_F(StaticConfigValidatorTest, Success) {
    const auto config = tests::MergeYaml(tests::kMinimalStaticConfig, kTestComponentConfigPatch);

    auto component_list = components::MinimalComponentList();
    component_list.Append<TestComponent>();

    UEXPECT_NO_THROW(components::RunOnce(components::InMemoryConfig{config}, component_list));
}

namespace {

constexpr std::string_view kTestComponentConfigPatchTypo = R"(
components_manager:
    components:
        test-component:
            # Note: typo in the option name
            sole-config-option: 42
)";

}  // namespace

TEST_F(StaticConfigValidatorTest, TypoInConfigOption) {
    const auto config = tests::MergeYaml(tests::kMinimalStaticConfig, kTestComponentConfigPatchTypo);

    auto component_list = components::MinimalComponentList();
    component_list.Append<TestComponent>();

    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{config}, component_list),
        std::exception,
        // TODO the message is a bit too cluttered.
        "The following components have failed static config validation:\n\t"
        "test-component: Error while validating static config against schema. "
        "Field 'components_manager.components.test-component.sole-config-option' is not declared "
        "in schema 'test-component' (declared: load-enabled, some-config-option). "
        "Perhaps you meant 'some-config-option'? "
        "You've probably made a typo or forgot to define GetStaticConfigSchema method for this component. "
        "If so, then please go and define this method according to the config options "
        "the component's constructor actually uses."
    );
}

namespace {

class WithoutSchemaTestComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "test-component";

    WithoutSchemaTestComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        [[maybe_unused]] const auto some_config_option = config["some-config-option"].As<int>();
    }

    // Notably, no GetStaticConfigSchema
};

}  // namespace

TEST_F(StaticConfigValidatorTest, OptionMissingInSchema) {
    const auto config = tests::MergeYaml(tests::kMinimalStaticConfig, kTestComponentConfigPatch);

    auto component_list = components::MinimalComponentList();
    component_list.Append<WithoutSchemaTestComponent>();

    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{config}, component_list),
        std::exception,
        // TODO the message is a bit too cluttered.
        "The following components have failed static config validation:\n\t"
        "test-component: Error while validating static config against schema. "
        "Field 'components_manager.components.test-component.some-config-option' is not declared "
        "in schema 'test-component' (declared: load-enabled). "
        "You've probably made a typo or forgot to define GetStaticConfigSchema method for this component. "
        "If so, then please go and define this method according to the config options "
        "the component's constructor actually uses."
    );
}

namespace {

class InvalidSchemaTestComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "test-component";

    using components::ComponentBase::ComponentBase;

    static yaml_config::Schema GetStaticConfigSchema() {
        // Invalid, because required 'additionalProperties' field is missed.
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
            type: object
            description: config of InvalidSchemaTestComponent
            properties:
                some-config-option:
                    type: integer
                    description: some-config-option description
        )");
    }
};

}  // namespace

TEST_F(StaticConfigValidatorTest, InvalidSchema) {
    const auto config = tests::MergeYaml(tests::kMinimalStaticConfig, kTestComponentConfigPatch);

    auto component_list = components::MinimalComponentList();
    component_list.Append<InvalidSchemaTestComponent>();

    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{config}, component_list),
        std::exception,
        "The following components have failed static config validation:\n\t"
        "test-component: Invalid static config schema: "
        "Schema field '/' of type 'object' must have field 'additionalProperties'"
    );
}

USERVER_NAMESPACE_END
