#include <userver/yaml_config/merge_schemas.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace yaml_config::impl {

void Merge(Schema& destination, Schema&& source) {
  if (source.type != FieldType::kObject) {
    throw std::runtime_error(fmt::format(
        "Error while merging schemas. Parent schema '{}' must have type "
        "'object', but type is '{}'",
        source.path, ToString(source.type)));
  }
  if (destination.type != FieldType::kObject) {
    throw std::runtime_error(fmt::format(
        "Error while merging schemas. Child schema '{}' must have type "
        "'object', but type is '{}'",
        destination.path, ToString(destination.type)));
  }

  auto& properties = destination.properties.value();
  for (auto& [name, source_item] : source.properties.value()) {
    auto it = properties.find(name);
    if (it != properties.end()) {
      auto& destination_item = *it->second;
      Merge(destination_item, std::move(*source_item));
    }
    properties.emplace(name, std::move(source_item));
  }
}

}  // namespace yaml_config::impl

USERVER_NAMESPACE_END
