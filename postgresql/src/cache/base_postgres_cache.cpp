#include <userver/cache/base_postgres_cache.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

std::string GetPostgreCacheSchema() {
  return R"(
type: object
description: Caching component for PostgreSQL derived from components::CachingComponentBase.
additionalProperties: false
properties:
    full-update-op-timeout:
        type: string
        description: timeout for a full update
        defaultDescription: 1m
    incremental-update-op-timeout:
        type: string
        description: timeout for an incremental update
        defaultDescription: 1s
    update-correction:
        type: string
        description: incremental update window adjustment
        defaultDescription: 0 for caches with defined GetLastKnownUpdated
    chunk-size:
        type: integer
        description: number of rows to request from PostgreSQL, 0 to fetch all rows in one request
        defaultDescription: 1000
    pgcomponent:
        type: string
        description: PostgreSQL component name
        defaultDescription: ""
)";
}

}  // namespace components::impl

USERVER_NAMESPACE_END
