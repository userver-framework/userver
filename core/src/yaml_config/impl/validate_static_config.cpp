#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config::impl {

namespace {

const std::string kFallbackSuffix = "#fallback";
const auto kFallbackSuffixLength = kFallbackSuffix.length();

std::string_view RemoveFallbackSuffix(std::string_view option) {
  if (boost::algorithm::ends_with(option, kFallbackSuffix)) {
    return option.substr(0, option.length() - kFallbackSuffixLength);
  }
  return option;
}

enum class FieldType {
  kInt,
  kString,
  kBool,
  kDouble,
  kObject,
  kArray,
};

const std::unordered_map<std::string, FieldType> kNamesToTypes{
    {"integer", FieldType::kInt},   {"string", FieldType::kString},
    {"boolean", FieldType::kBool},  {"double", FieldType::kDouble},
    {"object", FieldType::kObject}, {"array", FieldType::kArray}};

FieldType Parse(const formats::yaml::Value& type,
                formats::parse::To<FieldType>) {
  const std::string as_string = ToString(type);
  const auto it = kNamesToTypes.find(as_string);

  if (it != kNamesToTypes.end()) {
    return it->second;
  }

  throw std::runtime_error(
      fmt::format("Error while validating static config against schema. "
                  "Field 'type' must be one of ['integer', 'string' 'boolean', "
                  "'object', 'array']), but '{}' was given",
                  as_string));
}

std::string ToString(FieldType type) {
  switch (type) {
    case FieldType::kInt:
      return "integer";
    case FieldType::kString:
      return "string";
    case FieldType::kBool:
      return "boolean";
    case FieldType::kDouble:
      return "double";
    case FieldType::kObject:
      return "object";
    case FieldType::kArray:
      return "array";
  }

  UINVARIANT(false, "Unexpected FieldType");
}

bool IsTypeValid(FieldType type, const formats::yaml::Value& value) {
  switch (type) {
    case FieldType::kInt:
      return value.IsInt() || value.IsUInt64() || value.IsInt64();
    case FieldType::kString:
      return value.IsString();
    case FieldType::kBool:
      return value.IsBool();
    case FieldType::kDouble:
      return value.IsDouble();
    case FieldType::kObject:
      return value.IsObject();
    case FieldType::kArray:
      return value.IsArray();
  }

  UINVARIANT(false, "Incorrect field type");
}

void CheckType(FieldType type, const yaml_config::YamlConfig& value) {
  if (!IsTypeValid(type, value.Yaml())) {
    throw std::runtime_error(
        fmt::format("Error while validating static config against schema. "
                    "Value '{}' of field '{}' must be {}",
                    formats::yaml::ToString(value.Yaml()), value.GetPath(),
                    ToString(type)));
  }
}

void ValidateAndCheckScalars(const yaml_config::YamlConfig& static_config,
                             const formats::yaml::Value& schema) {
  if (!static_config.Yaml().IsObject() && !static_config.Yaml().IsArray()) {
    const auto type = schema["type"].As<FieldType>();
    CheckType(type, static_config);
    return;
  }

  Validate(static_config, schema);
}

void ValidateArray(const yaml_config::YamlConfig& array,
                   const formats::yaml::Value& schema) {
  const auto items = schema["items"];
  if (items.IsMissing()) {
    throw std::runtime_error(
        fmt::format("Schema field '{}' of component '{}' of type 'array' "
                    "must have field 'items'",
                    schema.GetPath(), array.GetPath()));
  }

  if (!array.Yaml().IsArray()) {
    throw std::runtime_error(
        fmt::format("Config field '{}' must be array. Schema path: '{}'",
                    array.GetPath(), items.GetPath()));
  }

  for (const auto& element : array) {
    ValidateAndCheckScalars(element, items);
  }
}

void ValidateObject(const yaml_config::YamlConfig& object,
                    const formats::yaml::Value& schema) {
  const auto properties = schema["properties"];
  if (properties.IsMissing()) {
    throw std::runtime_error(
        fmt::format("Schema field '{}' of component '{}' of type 'object' "
                    "must have field 'properties'",
                    schema.GetPath(), object.GetPath()));
  }

  if (!object.Yaml().IsObject()) {
    throw std::runtime_error(
        fmt::format("Config field '{}' must be object. Schema path: '{}'",
                    object.GetPath(), properties.GetPath()));
  }

  for (const auto& [name, value] : Items(object)) {
    const auto option = properties[RemoveFallbackSuffix(name)];
    if (option.IsMissing()) {
      throw std::runtime_error(fmt::format(
          "Field '{}' is not declared in schema '{}' of static config",
          value.GetPath(), properties.GetPath()));
    }
    ValidateAndCheckScalars(value, option);
  }
}

}  // namespace

void Validate(const yaml_config::YamlConfig& static_config,
              const formats::yaml::Value& schema) {
  if (schema.IsMissing()) {
    throw std::runtime_error(fmt::format(
        "Field '{}' is not declared in schema '{}' of static config",
        static_config.GetPath(), schema.GetPath()));
  }

  const auto type = schema["type"].As<FieldType>();

  CheckType(type, static_config);

  if (!schema.HasMember("description")) {
    throw std::runtime_error(fmt::format(
        "Schema field '{}' of component '{}' must have field 'description'",
        schema.GetPath(), static_config.GetPath()));
  }

  if (type == FieldType::kObject) {
    ValidateObject(static_config, schema);
  } else if (type == FieldType::kArray) {
    ValidateArray(static_config, schema);
  }
}

}  // namespace yaml_config::impl

USERVER_NAMESPACE_END
