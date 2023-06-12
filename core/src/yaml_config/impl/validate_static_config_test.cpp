#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/yaml_config/schema.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void Validate(const std::string& static_config, const std::string& schema) {
  yaml_config::impl::Validate(
      yaml_config::YamlConfig(formats::yaml::FromString(static_config), {}),
      yaml_config::impl::SchemaFromString(schema));
}

}  // namespace

TEST(StaticConfigValidator, IncorrectSchemaField) {
  const std::string kSchema = R"(
type: integer
description: with incorrect field name
incorrect_filed_name:
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field name must be one of ['type', 'description', "
      "'defaultDescription', 'additionalProperties', 'properties', 'items', "
      "'enum', 'minimum', 'maximum'], but 'incorrect_filed_name' was given. "
      "Schema path: '/'");
}

TEST(StaticConfigValidator, AdditionalPropertiesAbsent) {
  const std::string kSchema = R"(
type: object
description: object without additionalProperties
properties: {}
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field '/' of type 'object' must have field "
      "'additionalProperties'");
}

TEST(StaticConfigValidator, AdditionalProperties) {
  const std::string kSchema = R"(
type: object
description: object with integer additionalProperties
additionalProperties:
    type: integer
    description: integer value
properties:
    declared:
        type: string
        description: declared property
)";
  const std::string kStaticConfig = R"(
a: 1
declared: abc
b: 2
c: abc
d: 3
)";

  UEXPECT_THROW_MSG(
      Validate(kStaticConfig, kSchema), std::runtime_error,
      "Error while validating static config against schema. Value "
      "'abc' of field 'c' must be integer");
}

TEST(StaticConfigValidator, AdditionalPropertiesTrue) {
  const std::string kSchema = R"(
type: object
description: object with additionalProperties set to 'true'
additionalProperties: true
properties: {}
)";

  const std::string kStaticConfig = R"(
a: 42
b: [1, 2, 3]
c:
    d: abc
)";
  UEXPECT_NO_THROW(Validate(kStaticConfig, kSchema));
}

TEST(StaticConfigValidator, PropertiesAbsent) {
  const std::string kSchema = R"(
type: object
description: object without properties
additionalProperties: false
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field '/' of type 'object' must have field 'properties'");
}

TEST(StaticConfigValidator, ItemsAbsent) {
  const std::string kSchema = R"(
type: array
description: array without items
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field '/' of type 'array' must have field 'items'");
}

TEST(StaticConfigValidator, ItemsOutOfArray) {
  const std::string kSchema = R"(
type: string
description: string with items
items:
    type: integer
    description: element description
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field '/' of type 'string' can not have field "
      "'items', because its type is not 'array'");
}

TEST(StaticConfigValidator, PropertiesOutOfObject) {
  const std::string kSchema = R"(
type: integer
description: integer with properties
properties: {}
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field '/' of type 'integer' can not have field "
      "'properties', because its type is not 'object'");
}

TEST(StaticConfigValidator, AdditionalPropertiesOutOfObject) {
  const std::string kSchema = R"(
type: integer
description: integer with additionalProperties
additionalProperties: false
)";

  UEXPECT_THROW_MSG(
      formats::yaml::FromString(kSchema).As<yaml_config::Schema>(),
      std::runtime_error,
      "Schema field '/' of type 'integer' can not have field "
      "'additionalProperties', because its type is not 'object'");
}

TEST(StaticConfigValidator, Integer) {
  const std::string kStaticConfig = R"(
42
)";
  const std::string kSchema = R"(
type: integer
description: answer to the ultimate question
)";
  UEXPECT_NO_THROW(Validate(kStaticConfig, kSchema));
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
description: server description
additionalProperties: false
properties:
    listener:
        type: object
        description: listener description
        additionalProperties: false
        properties:
            port:
                type: integer
                description: port description
            connection:
                type: object
                description: connection description
                additionalProperties: false
                properties:
                    in_buffer_size:
                        type: integer
                        description: in_buffer_size description
)";

  UEXPECT_THROW_MSG(
      Validate(kStaticConfig, kSchema), std::runtime_error,
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
description: simple array
additionalProperties: false
properties:
    arr:
        type: array
        description: integer array
        items:
            type: integer
            description: element of array
)";
  UEXPECT_THROW_MSG(
      Validate(kStaticConfig, kSchema), std::runtime_error,
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
description: array description
additionalProperties: false
properties:
    arr:
        type: array
        description: key-value array
        items:
            type: object
            description: element description
            additionalProperties: false
            properties:
                key:
                    type: string
                    description: key description
                value:
                    type: integer
                    description: value description
)";
  UEXPECT_THROW_MSG(
      Validate(kStaticConfig, kSchema), std::runtime_error,
      "Error while validating static config against schema. Field "
      "'arr[1].not_declared_option' is not declared in schema "
      "'properties.arr.items'");
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
description: recursive description
additionalProperties: false
properties:
    huge-object:
        type: object
        description: huge-object description
        additionalProperties: false
        properties:
            big-object:
                type: object
                description: big-object description
                additionalProperties: false
                properties:
                    key:
                        type: string
                        description: key description
                    value:
                        type: integer
                        description: value description
                    arrays:
                        type: object
                        description: arrays description
                        additionalProperties: false
                        properties:
                            simple-array:
                                type: array
                                description: integer array
                                items:
                                    type: integer
                                    description: element description
                            key-value-array:
                                type: array
                                description: key-value array
                                items:
                                    type: object
                                    description: element description
                                    additionalProperties: false
                                    properties:
                                        key:
                                            type: string
                                            description: key description
                                        value:
                                            type: integer
                                            description: value description
)";
  UEXPECT_NO_THROW(Validate(kStaticConfig, kSchema));
}

TEST(StaticConfigValidator, Enum) {
  const std::string kCorrectStaticConfig = R"(
mode: on
)";

  const std::string kMissingEnumStaticConfig = R"(
mode:
)";

  const std::string kIncorrectEnumValueStaticConfig = R"(
mode: not declared enum value
)";

  const std::string kIncorrectTypeStaticConfig = R"(
mode: []
)";

  const std::string kSchema = R"(
type: object
description: for enum test
additionalProperties: false
properties:
    mode:
        type: string
        description: mode on or off
        enum:
          - on
          - off
)";

  UEXPECT_NO_THROW(Validate(kCorrectStaticConfig, kSchema));

  UEXPECT_NO_THROW(Validate(kMissingEnumStaticConfig, kSchema));

  UEXPECT_THROW_MSG(Validate(kIncorrectEnumValueStaticConfig, kSchema),
                    std::runtime_error,
                    "Error while validating static config against schema. Enum "
                    "field 'not declared enum value' must be one of [off, on]");

  UEXPECT_THROW_MSG(Validate(kIncorrectTypeStaticConfig, kSchema),
                    std::runtime_error,
                    "Error while validating static config against schema. "
                    "Value '[]' of field 'mode' must be string");
}

TEST(StaticConfigValidator, IntegerBounds) {
  const std::string kSchema = R"(
type: integer
description: .
minimum: 10
maximum: 20
)";

  UEXPECT_NO_THROW(Validate("10", kSchema));
  UEXPECT_NO_THROW(Validate("15", kSchema));
  UEXPECT_NO_THROW(Validate("20", kSchema));
  UEXPECT_THROW_MSG(Validate("9", kSchema), std::runtime_error,
                    "Expected integer at path '/' to be >= 10 (actual: 9)");
  UEXPECT_THROW_MSG(Validate("21", kSchema), std::runtime_error,
                    "Expected integer at path '/' to be <= 20 (actual: 21)");
  UEXPECT_THROW_MSG(Validate("15.5", kSchema), std::runtime_error,
                    "Value '15.5' of field '/' must be integer");
  UEXPECT_THROW_MSG(Validate("What", kSchema), std::runtime_error,
                    "Value 'What' of field '/' must be integer");
}

TEST(StaticConfigValidatorBounds, NumberBounds) {
  const std::string kSchema = R"(
type: number
description: .
minimum: 10.5
maximum: 19.5
)";

  UEXPECT_NO_THROW(Validate("15", kSchema));
  UEXPECT_NO_THROW(Validate("10.5", kSchema));
  UEXPECT_NO_THROW(Validate("10.6", kSchema));
  UEXPECT_NO_THROW(Validate("19.4", kSchema));
  UEXPECT_NO_THROW(Validate("19.5", kSchema));
  UEXPECT_THROW_MSG(Validate("10.4", kSchema), std::runtime_error,
                    "Expected number at path '/' to be >= 10.5 (actual: 10.4)");
  UEXPECT_THROW_MSG(Validate("19.6", kSchema), std::runtime_error,
                    "Expected number at path '/' to be <= 19.5 (actual: 19.6)");
  UEXPECT_THROW_MSG(Validate("What", kSchema), std::runtime_error,
                    "Value 'What' of field '/' must be number");
}

USERVER_NAMESPACE_END
