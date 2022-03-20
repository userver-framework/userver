#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/yaml_config/schema.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

yaml_config::Schema Merge(const std::string& parent, const std::string& child) {
  yaml_config::Schema parent_schema(parent);
  yaml_config::Schema child_schema(child);
  yaml_config::Merge(child_schema, std::move(parent_schema));
  return child_schema;
}

void CheckMergingFail(const std::string& parent, const std::string& child,
                      const std::string& expected_message) {
  try {
    Merge(parent, child);
    FAIL() << "Should have thrown";
  } catch (const std::runtime_error& exception) {
    EXPECT_EQ(exception.what(), expected_message);
  } catch (const std::exception& exception) {
    FAIL() << "Expect runtime error. Message: " << exception.what();
  }
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
  CheckMergingFail(
      kIntegerSchema, kChildSchema,
      "Error while merging schemas. Parent schema '/' must have type "
      "'object', but type is 'integer'");
}

TEST(MergeSchemas, ChildNoObject) {
  CheckMergingFail(
      kParentSchema, kIntegerSchema,
      "Error while merging schemas. Child schema '/' must have type "
      "'object', but type is 'integer'");
}

TEST(MergeSchemas, IncorrectCommonOption) {
  CheckMergingFail(kClashingSchema, kParentSchema,
                   "Error while merging schemas. Parent schema "
                   "'properties.common_option.properties.parent_option' must "
                   "have type 'object', but type is 'integer'");
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
  EXPECT_TRUE(common_option_properties.find("parent_option") !=
              common_option_properties.end());
  EXPECT_TRUE(common_option_properties.find("child_option") !=
              common_option_properties.end());
}

USERVER_NAMESPACE_END
