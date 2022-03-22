#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <userver/formats/yaml_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

struct Schema;

enum class FieldType {
  kInt,
  kString,
  kBool,
  kDouble,
  kObject,
  kArray,
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

struct Schema final {
  Schema() = default;

  explicit Schema(const std::string& yaml_string);

  void UpdateDescription(std::string new_description);

  std::string path;

  FieldType type{};
  std::string description;
  std::optional<std::string> default_description;
  std::optional<std::variant<bool, SchemaPtr>> additional_properties;
  std::optional<std::unordered_map<std::string, SchemaPtr>> properties;
  std::optional<SchemaPtr> items;
};

Schema Parse(const formats::yaml::Value& schema, formats::parse::To<Schema>);

}  // namespace yaml_config

USERVER_NAMESPACE_END
