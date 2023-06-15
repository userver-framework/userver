#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <fmt/format.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptors.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/schema.hpp>
#include <utils/distances.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config::impl {

namespace {

constexpr std::string_view kFallbackSuffix = "#fallback";
constexpr std::string_view kEnvSuffix = "#env";

std::string RemoveFallbackAndEnvSuffix(std::string_view option) {
  if (boost::algorithm::ends_with(option, kFallbackSuffix)) {
    return std::string(
        option.substr(0, option.length() - kFallbackSuffix.length()));
  }
  if (boost::algorithm::ends_with(option, kEnvSuffix)) {
    return std::string(option.substr(0, option.length() - kEnvSuffix.length()));
  }
  return std::string(option);
}

bool IsTypeValid(FieldType type, const formats::yaml::Value& value) {
  switch (type) {
    case FieldType::kInteger:
      return value.IsInt() || value.IsUInt64() || value.IsInt64();
    case FieldType::kString:
      return value.IsString();
    case FieldType::kBool:
      return value.IsBool();
    case FieldType::kNumber:
      return value.IsDouble();
    case FieldType::kObject:
      return value.IsObject() || value.IsNull();
    case FieldType::kArray:
      return value.IsArray() || value.IsNull();
    default:
      UINVARIANT(false, "Incorrect field type");
  }
}

void CheckType(const YamlConfig& value, const Schema& schema) {
  if (!IsTypeValid(schema.type, value.Yaml())) {
    throw std::runtime_error(
        fmt::format("Error while validating static config against schema. "
                    "Value '{}' of field '{}' must be {}",
                    formats::yaml::ToString(value.Yaml()), value.GetPath(),
                    ToString(schema.type)));
  }
}

void ValidateEnum(const YamlConfig& enum_value, const Schema& schema) {
  CheckType(enum_value, schema);
  if (schema.enum_values->find(enum_value.As<std::string>()) ==
      schema.enum_values->end()) {
    std::vector<std::string> ordered_enum_values(
        schema.enum_values.value().begin(), schema.enum_values.value().end());
    std::sort(ordered_enum_values.begin(), ordered_enum_values.end());

    throw std::runtime_error(fmt::format(
        "Error while validating static config against schema. "
        "Enum field '{}' must be one of [{}]",
        enum_value.As<std::string>(), fmt::join(ordered_enum_values, ", ")));
  }
}

void ValidateIfPresent(const YamlConfig& static_config, const Schema& schema) {
  if (!static_config.IsMissing() && !static_config.IsNull()) {
    Validate(static_config, schema);
  }
}

std::string KeysAsString(
    const std::unordered_map<std::string, SchemaPtr>& object) {
  return fmt::format("{}", fmt::join(object | boost::adaptors::map_keys, ", "));
}

void ValidateObject(const YamlConfig& object, const Schema& schema) {
  const auto& properties = schema.properties.value();

  for (const auto& [name, value] : Items(object)) {
    const std::string search_name = RemoveFallbackAndEnvSuffix(name);
    if (const auto it = properties.find(search_name); it != properties.end()) {
      ValidateIfPresent(value, *it->second);
      continue;
    }

    const auto& additional_properties = schema.additional_properties.value();
    if (std::holds_alternative<SchemaPtr>(additional_properties)) {
      ValidateIfPresent(value, *std::get<SchemaPtr>(additional_properties));
      continue;
    } else if (std::get<bool>(additional_properties)) {
      continue;
    }

    // additionalProperties: false
    throw std::runtime_error(fmt::format(
        "Error while validating static config against schema. "
        "Field '{}' is not declared in schema '{}' (declared: {}). "
        "You've probably "
        "made a typo or forgot to define components' static config schema.{}",
        value.GetPath(), schema.path, KeysAsString(properties),
        utils::SuggestNearestName(properties | boost::adaptors::map_keys,
                                  search_name)));
  }
}

void ValidateArray(const YamlConfig& array, const Schema& schema) {
  for (const auto& element : array) {
    ValidateIfPresent(element, *schema.items.value());
  }
}

void CheckNumericBounds(const YamlConfig& value, const Schema& schema) {
  if (schema.minimum && !(value.As<double>() >= *schema.minimum)) {
    throw std::runtime_error(
        fmt::format("Error while validating static config against schema. "
                    "Expected {} at path '{}' to be >= {} (actual: {}).",
                    ToString(schema.type), value.GetPath(), *schema.minimum,
                    formats::yaml::ToString(value.Yaml())));
  }

  if (schema.maximum && !(value.As<double>() <= *schema.maximum)) {
    throw std::runtime_error(
        fmt::format("Error while validating static config against schema. "
                    "Expected {} at path '{}' to be <= {} (actual: {}).",
                    ToString(schema.type), value.GetPath(), *schema.maximum,
                    formats::yaml::ToString(value.Yaml())));
  }
}

}  // namespace

void Validate(const YamlConfig& static_config, const Schema& schema) {
  CheckType(static_config, schema);

  if (schema.type == FieldType::kObject) {
    ValidateObject(static_config, schema);
  } else if (schema.type == FieldType::kArray) {
    ValidateArray(static_config, schema);
  } else if (schema.enum_values.has_value()) {
    ValidateEnum(static_config, schema);
  }

  if (schema.type == FieldType::kInteger || schema.type == FieldType::kNumber) {
    CheckNumericBounds(static_config, schema);
  }
}

}  // namespace yaml_config::impl

USERVER_NAMESPACE_END
