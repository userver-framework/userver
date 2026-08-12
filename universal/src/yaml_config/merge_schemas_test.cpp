#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/yaml_config/schema.hpp>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

yaml_config::Schema Merge(const std::string& parent, const std::string& child) {
    auto parent_schema = yaml_config::impl::SchemaFromString(parent);
    auto child_schema = yaml_config::impl::SchemaFromString(child);
    yaml_config::impl::Merge(child_schema, std::move(parent_schema));
    return child_schema;
}

const std::string kIntegerSchema = R"(
type: integer
description: .
)";

const std::string kParentSchema = R"(
type: object
description: parent config
additionalProperties: false
properties:
    parent_option:
        type: integer
        description: .
    common_option:
        type: object
        description: parent common_option
        additionalProperties: false
        properties:
            parent_option:
                type: integer
                description: .
)";

const std::string kChildSchema = R"(
type: object
description: child config
additionalProperties: false
properties:
    child_option:
        type: integer
        description: .
    common_option:
        type: object
        description: child common_option
        additionalProperties: false
        properties:
            child_option:
                type: integer
                description: .
)";

const std::string kClashingSchema = R"(
type: object
description: parent config
additionalProperties: false
properties:
    common_option:
        type: object
        description: parent common_option
        additionalProperties: false
        properties:
            parent_option:
                type: integer
                description: .
)";

}  // namespace

TEST(MergeSchemas, ParentNoObject) {
    UEXPECT_THROW_MSG(
        Merge(kIntegerSchema, kChildSchema),
        std::runtime_error,
        "Error while merging schemas. Parent schema '/' must have type "
        "'object', but type is 'integer'"
    );
}

TEST(MergeSchemas, ChildNoObject) {
    UEXPECT_THROW_MSG(
        Merge(kParentSchema, kIntegerSchema),
        std::runtime_error,
        "Error while merging schemas. Child schema '/' must have type "
        "'object', but type is 'integer'"
    );
}

TEST(MergeSchemas, IncorrectCommonOption) {
    UEXPECT_THROW_MSG(
        Merge(kClashingSchema, kParentSchema),
        std::runtime_error,
        "Error while merging schemas. Parent schema "
        "'properties.common_option.properties.parent_option' must "
        "have type 'object', but type is 'integer'"
    );
}

TEST(MergeSchemas, CommonOption) {
    const auto schema = Merge(kParentSchema, kChildSchema);
    EXPECT_EQ(schema.description, "child config");

    const auto& properties = schema.properties.value();
    EXPECT_TRUE(properties.find("parent_option") != properties.end());
    EXPECT_TRUE(properties.find("child_option") != properties.end());
    EXPECT_TRUE(properties.find("common_option") != properties.end());

    const auto& common_option = *properties.at("common_option");
    EXPECT_EQ(common_option.description, "child common_option");

    const auto& common_option_properties = common_option.properties.value();
    EXPECT_TRUE(common_option_properties.find("parent_option") != common_option_properties.end());
    EXPECT_TRUE(common_option_properties.find("child_option") != common_option_properties.end());
}

// Parent schema with required: [parent_option]
const std::string kRequiredParentSchema = R"(
type: object
description: parent config
additionalProperties: false
properties:
    parent_option:
        type: integer
        description: .
required:
  - parent_option
)";

// Parent schema without required
const std::string kNoRequiredParentSchema = R"(
type: object
description: parent config
additionalProperties: false
properties:
    parent_option:
        type: integer
        description: .
)";

// Child schema with required: [child_option]
const std::string kRequiredChildSchema = R"(
type: object
description: child config
additionalProperties: false
properties:
    child_option:
        type: integer
        description: .
required:
  - child_option
)";

// Child schema without required
const std::string kNoRequiredChildSchema = R"(
type: object
description: child config
additionalProperties: false
properties:
    child_option:
        type: integer
        description: .
)";

TEST(MergeSchemas, RequiredUnion) {
    // Both parent and child have required — result is the union.
    // parent_option comes from parent (not redeclared in child, Merge injects it).
    const std::string parent = R"(
type: object
description: parent config
additionalProperties: false
properties:
    parent_option:
        type: integer
        description: .
    shared_option:
        type: string
        description: .
required:
  - parent_option
)";

    const auto schema = Merge(parent, kRequiredChildSchema);
    ASSERT_TRUE(schema.required.has_value());
    EXPECT_TRUE(schema.required->count("parent_option") == 1);
    EXPECT_TRUE(schema.required->count("child_option") == 1);
    EXPECT_EQ(schema.required->size(), 2);
}

TEST(MergeSchemas, RequiredChildOnlyNoParentRequired) {
    // parent has no required; child has required; result keeps child's required.
    const auto schema = Merge(kNoRequiredParentSchema, kRequiredChildSchema);
    ASSERT_TRUE(schema.required.has_value());
    EXPECT_EQ(*schema.required, (std::unordered_set<std::string>{"child_option"}));
}

TEST(MergeSchemas, RequiredParentOnlyNoChildRequired) {
    // parent has required; child has no required; result gains parent's required.
    const auto schema = Merge(kRequiredParentSchema, kNoRequiredChildSchema);
    ASSERT_TRUE(schema.required.has_value());
    EXPECT_EQ(*schema.required, (std::unordered_set<std::string>{"parent_option"}));
}

TEST(YamlConfigSchema, AdditionalPropertiesFalse) {
    auto schema = yaml_config::impl::SchemaFromString(R"(
type: object
description: test
additionalProperties: false
properties: {}
)");
    ASSERT_TRUE(schema.additional_properties.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(schema.additional_properties.value()));
    EXPECT_FALSE(std::get<bool>(schema.additional_properties.value()));
}

USERVER_NAMESPACE_END
