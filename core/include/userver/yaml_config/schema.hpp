#pragma once

/// @file userver/yaml_config/schema.hpp
/// @brief @copybrief yaml_config::Schema

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <userver/formats/yaml_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

struct Schema;

enum class FieldType {
  kBool,
  kInteger,
  kNumber,
  kString,
  kArray,
  kObject,
};

std::string ToString(FieldType type);

class SchemaPtr final {
 public:
  explicit SchemaPtr(Schema&& schema);

  const Schema& operator*() const { return *schema_; }
  Schema& operator*() { return *schema_; }

 private:
  std::unique_ptr<Schema> schema_;
};

/// @brief JSON Schema-like type definition
///
/// @see @ref static-configs-validation "Static configs validation"
struct Schema final {
  void UpdateDescription(std::string new_description);

  static Schema EmptyObject();

  std::string path;

  FieldType type{};
  std::string description;
  std::optional<std::string> default_description;
  std::optional<std::variant<bool, SchemaPtr>> additional_properties;
  std::optional<std::unordered_map<std::string, SchemaPtr>> properties;
  std::optional<SchemaPtr> items;
  std::optional<std::unordered_set<std::string>> enum_values;
  std::optional<double> minimum;
  std::optional<double> maximum;
};

Schema Parse(const formats::yaml::Value& schema, formats::parse::To<Schema>);

namespace impl {

Schema SchemaFromString(const std::string& yaml_string);

}  // namespace impl

}  // namespace yaml_config

USERVER_NAMESPACE_END
