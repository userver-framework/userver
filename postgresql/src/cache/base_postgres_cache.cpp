#include <userver/cache/base_postgres_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

std::string GetStaticConfigSchema() {
  return R"(
type: object
description: postgre cache config
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
        defaultDescription: - (0 for caches with defined GetLastKnownUpdated)
    chunk-size:
        type: integer
        description: number of rows to request from PostgreSQL, 0 to fetch all rows in one request
        defaultDescription: 1000
)";
}

}  // namespace components::impl

USERVER_NAMESPACE_END
