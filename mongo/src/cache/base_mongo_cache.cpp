#include <userver/cache/base_mongo_cache.hpp>

#include <userver/components/component_config.hpp>
#include <userver/utils/resources.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/cache/base_mongo_cache.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components::impl {

std::chrono::milliseconds GetMongoCacheUpdateCorrection(const ComponentConfig& config) {
    return config["update-correction"].As<std::chrono::milliseconds>(0);
}

std::string GetMongoCacheSchema() { return utils::FindResource("src/cache/base_mongo_cache.yaml"); }

}  // namespace components::impl

USERVER_NAMESPACE_END
