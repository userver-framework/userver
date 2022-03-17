#include <userver/cache/base_mongo_cache.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

std::chrono::milliseconds GetMongoCacheUpdateCorrection(
    const ComponentConfig& config) {
  return config["update-correction"].As<std::chrono::milliseconds>(0);
}

yaml_config::Schema GetChildSchema() {
  return yaml_config::Schema(R"(
type: object
description: mongo-cache config
additionalProperties: false
properties:
    update-correction:
        type: string
        description: adjusts incremental updates window to overlap with previous update
        defaultDescription: 0
)");
}

}  // namespace components::impl

USERVER_NAMESPACE_END
