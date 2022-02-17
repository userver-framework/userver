#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/server/component.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

void Validate(const std::string& static_config, const std::string& schema) {
  yaml_config::impl::Validate(
      yaml_config::YamlConfig(formats::yaml::FromString(static_config), {}),
      formats::yaml::FromString(schema));
}

void CheckFail(const std::string& static_config, const std::string& schema,
               const std::string& expected_message) {
  try {
    Validate(static_config, schema);
    FAIL() << "Should have thrown";
  } catch (const std::runtime_error& exception) {
    EXPECT_EQ(exception.what(), expected_message);
  } catch (const std::exception& exception) {
    FAIL() << "Expect runtime error. Message: " << exception.what();
  }
}

}  // namespace

TEST(StaticConfigValidator, Integer) {
  const std::string kStaticConfig = R"(
42
)";
  const std::string kSchema = R"(
type: integer
description:)";
  Validate(kStaticConfig, kSchema);
}

TEST(StaticConfigValidator, RecursiveFailed) {
  const std::string kStaticConfig = R"(
listener:
    port: 0
    connection:
        in_buffer_size: abc # must be integer
)";

  const std::string kSchema = R"(
type: object
description:
properties:
    listener:
        type: object
        description:
        properties:
            port:
                type: integer
                description:
            connection:
                type: object
                description:
                properties:
                    in_buffer_size:
                        type: integer
                        description
)";

  CheckFail(kStaticConfig, kSchema,
            "Error while validating static config against schema. Value 'abc' "
            "of field 'listener.connection.in_buffer_size' must be "
            "integer");
}

TEST(StaticConfigValidator, SimpleArrayFailed) {
  const std::string kStaticConfig = R"(
arr: [2, 4, 6, abc]
)";
  const std::string kSchema = R"(
type: object
description:
properties:
    arr:
        type: array
        description: integer array
        items:
            type: integer
            description: element of array
)";
  CheckFail(kStaticConfig, kSchema,
            "Error while validating static config against schema. Value 'abc' "
            "of field 'arr[3]' must be integer");
}

TEST(StaticConfigValidator, ArrayFailed) {
  const std::string kStaticConfig = R"(
arr:
  - key: a
    value: 1
  - key: a
    value: 1
    not_declared_option:
)";
  const std::string kSchema = R"(
type: object
description:
properties:
    arr:
        type: array
        description: key-value array
        items:
            type: object
            description:
            properties:
                key:
                    type: string
                    description: key description
                value:
                    type: integer
                    description: value description
)";
  CheckFail(kStaticConfig, kSchema,
            "Field 'arr[1].not_declared_option' is not declared in schema "
            "'properties.arr.items.properties' of static config");
}

TEST(StaticConfigValidator, Recursive) {
  const std::string kStaticConfig = R"(
huge-object:
    big-object:
        key: a
        value: 1
        arrays:
            simple-array: [2, 4, 6]
            key-value-array:
              - key: a
                value: 1
              - key: b
                value: 2
)";
  const std::string kSchema = R"(
type: object
description:
properties:
    huge-object:
        type: object
        description:
        properties:
            big-object:
                type: object
                description:
                properties:
                    key:
                        type: string
                        description:
                    value:
                        type: integer
                        description:
                    arrays:
                        type: object
                        description:
                        properties:
                            simple-array:
                                type: array
                                description: integer array
                                items:
                                    type: integer
                            key-value-array:
                                type: array
                                description: key-value array
                                items:
                                    type: object
                                    description:
                                    properties:
                                        key:
                                            type: string
                                            description: key description
                                        value:
                                            type: integer
                                            description: value description
)";
  Validate(kStaticConfig, kSchema);
}

USERVER_NAMESPACE_END
