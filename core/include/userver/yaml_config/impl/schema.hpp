#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config::impl {

enum class FieldType {
  kInt,
  kString,
  kBool,
  kDouble,
  kObject,
  kArray,
};

std::string ToString(FieldType type);

struct Schema;

class SchemaPtr final {
 public:
  explicit SchemaPtr(Schema&& schema);

  const Schema& operator*() const { return *schema_; }

 private:
  std::unique_ptr<Schema> schema_;
};

struct Schema final {
  std::string path;

  FieldType type{};
  std::string description;
  std::optional<std::string> default_description;
  std::optional<bool> additional_properties;
  std::optional<std::unordered_map<std::string, SchemaPtr>> properties;
  std::optional<SchemaPtr> items;
};

Schema Parse(const formats::yaml::Value& schema, formats::parse::To<Schema>);

}  // namespace yaml_config::impl

USERVER_NAMESPACE_END
