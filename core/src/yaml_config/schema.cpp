#include <userver/yaml_config/schema.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

namespace {

void CheckFieldsNames(const formats::yaml::Value& yaml_schema) {
  static constexpr utils::TrivialSet kFieldNames = [](auto selector) {
    return selector()
        .Case("type")
        .Case("description")
        .Case("defaultDescription")
        .Case("additionalProperties")
        .Case("properties")
        .Case("items")
        .Case("enum");
  };

  for (const auto& [name, value] : Items(yaml_schema)) {
    const auto found = kFieldNames.Contains(name);

    if (!found) {
      throw std::runtime_error(
          fmt::format("Schema field name must be one of [{}], but '{}' was "
                      "given. Schema path: '{}'",
                      kFieldNames.Describe(), name, yaml_schema.GetPath()));
    }
  }
}

void CheckSchemaStructure(const Schema& schema) {
  if (schema.items.has_value() && schema.type != FieldType::kArray) {
    throw std::runtime_error(
        fmt::format("Schema field '{}' of type '{}' can not have field "
                    "'items', because its type is not 'array'",
                    schema.path, ToString(schema.type)));
  }
  if (schema.type != FieldType::kObject) {
    if (schema.properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type '{}' can not have field "
                      "'properties', because its type is not 'object'",
                      schema.path, ToString(schema.type)));
    }
    if (schema.additional_properties.has_value()) {
      throw std::runtime_error(fmt::format(
          "Schema field '{}' of type '{}' can not have field "
          "'additionalProperties', because its type is not 'object'",
          schema.path, ToString(schema.type)));
    }
  }

  if (schema.type == FieldType::kObject) {
    if (!schema.properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type 'object' "
                      "must have field 'properties'",
                      schema.path));
    }
    if (!schema.additional_properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type 'object' must have field "
                      "'additionalProperties'",
                      schema.path));
    }
  } else if (schema.type == FieldType::kArray) {
    if (!schema.items.has_value()) {
      throw std::runtime_error(fmt::format(
          "Schema field '{}' of type 'array' must have field 'items'",
          schema.path));
    }
  }
  if (schema.enum_values.has_value() && schema.type != FieldType::kString) {
    throw std::runtime_error(
        fmt::format("Schema field '{}' of type '{}' can not have field 'enum', "
                    "because its type is not 'string'",
                    schema.path, ToString(schema.type)));
  }
}

constexpr utils::TrivialBiMap kFieldTypes = [](auto selector) {
  return selector()
      .Case("integer", FieldType::kInt)
      .Case("string", FieldType::kString)
      .Case("boolean", FieldType::kBool)
      .Case("double", FieldType::kDouble)
      .Case("object", FieldType::kObject)
      .Case("array", FieldType::kArray);
};

}  // namespace

std::string ToString(FieldType type) {
  auto value = kFieldTypes.TryFind(type);
  UINVARIANT(value, "Incorrect field type");
  return std::string{*value};
}

FieldType Parse(const formats::yaml::Value& type,
                formats::parse::To<FieldType>) {
  const std::string as_string = type.As<std::string>();
  auto value = kFieldTypes.TryFind(as_string);

  if (value) {
    return *value;
  }

  throw std::runtime_error(fmt::format(
      "Schema field 'type' must be one of [{}]), but '{}' was given",
      kFieldTypes.DescribeFirst(), as_string));
}

SchemaPtr Parse(const formats::yaml::Value& schema,
                formats::parse::To<SchemaPtr>) {
  return SchemaPtr(schema.As<Schema>());
}

SchemaPtr::SchemaPtr(Schema&& schema)
    : schema_(std::make_unique<Schema>(std::move(schema))) {}

std::variant<bool, SchemaPtr> Parse(
    const formats::yaml::Value& value,
    formats::parse::To<std::variant<bool, SchemaPtr>>) {
  if (value.IsBool()) {
    return value.As<bool>();
  } else {
    return value.As<SchemaPtr>();
  }
}

Schema Parse(const formats::yaml::Value& schema, formats::parse::To<Schema>) {
  Schema result;
  result.path = schema.GetPath();
  result.type = schema["type"].As<FieldType>();
  result.description = schema["description"].As<std::string>();

  result.additional_properties =
      schema["additionalProperties"]
          .As<std::optional<std::variant<bool, SchemaPtr>>>();
  result.default_description =
      schema["defaultDescription"].As<std::optional<std::string>>();
  result.properties =
      schema["properties"]
          .As<std::optional<std::unordered_map<std::string, SchemaPtr>>>();
  result.items = schema["items"].As<std::optional<SchemaPtr>>();
  result.enum_values =
      schema["enum"].As<std::optional<std::unordered_set<std::string>>>();

  CheckFieldsNames(schema);

  CheckSchemaStructure(result);

  return result;
}

void Schema::UpdateDescription(std::string new_description) {
  description = std::move(new_description);
}

Schema Schema::EmptyObject() {
  return impl::SchemaFromString(R"(
  type: object
  properties: {}
  additionalProperties: false
  description: TODO
)");
}

Schema impl::SchemaFromString(const std::string& yaml_string) {
  return formats::yaml::FromString(yaml_string).As<Schema>();
}

}  //  namespace yaml_config

USERVER_NAMESPACE_END
