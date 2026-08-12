#include <userver/cache/base_postgres_cache.hpp>

#include <userver/utils/resources.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/cache/base_postgres_cache.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components::impl {

std::string GetPostgreCacheSchema() { return utils::FindResource("src/cache/base_postgres_cache.yaml"); }

}  // namespace components::impl

USERVER_NAMESPACE_END
