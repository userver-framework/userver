#pragma once

/// @file userver/yaml_config/merge_schemas.hpp
/// @brief @copybrief yaml_config::MergeSchemas

#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

namespace impl {

void Merge(Schema& destination, Schema&& source);

}  // namespace impl

/// @brief Merge parent and child components schemas of static configs
///
/// @see @ref static-configs-validation "Static configs validation"
template <typename ParentComponent>
Schema MergeSchemas(const std::string& yaml_string) {
  auto schema = impl::SchemaFromString(yaml_string);
  impl::Merge(schema, ParentComponent::GetStaticConfigSchema());
  return schema;
}

}  // namespace yaml_config

USERVER_NAMESPACE_END
