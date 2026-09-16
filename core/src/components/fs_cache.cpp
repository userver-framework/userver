#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/fs_cache.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/fs_cache.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

const FsCache::Client& FsCache::GetClient() const { return client_; }

FsCache::FsCache(const components::ComponentConfig& config, const components::ComponentContext& context)
    : components::ComponentBase(config, context),
      client_(fs::FsCacheClient::Settings{
          .dir = config["dir"].As<std::string>("/var/www"),
          .update_period = config["update-period"].As<std::chrono::milliseconds>(0),
          .task_processor = GetFsTaskProcessor(config, context),
          .max_size_to_cache = config["max-size-to-cache"].As<std::size_t>(fs::kDefaultMaxSizeToCache),
      })
{}

yaml_config::Schema FsCache::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/components/fs_cache.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
