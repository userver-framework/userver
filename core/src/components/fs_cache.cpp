#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/fs_cache.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

const FsCache::Client& FsCache::GetClient() const { return client_; }

FsCache::FsCache(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      client_(
          config["dir"].As<std::string>("/var/www"),
          config["update-period"].As<std::chrono::milliseconds>(0),
          context.GetTaskProcessor(config["fs-task-processor"].As<std::string>(
              "fs-task-processor"))) {}

yaml_config::Schema FsCache::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: component fs cache storage
additionalProperties: false
properties:
    dir:
        type: string
        description: directory to cache files from
        defaultDescription: /var/www
    update-period:
        type: string
        description: |
            update period (0 - fill the cache only at startup)
        defaultDescription: 0
    fs-task-processor:
        type: string
        description: task processor to do filesystem operations
        defaultDescription: fs-task-processor
)");
}

}  // namespace components

USERVER_NAMESPACE_END
