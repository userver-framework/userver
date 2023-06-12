#include <userver/cache/caching_component_base.hpp>

#include <userver/dump/dumper.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

yaml_config::Schema GetCachingComponentBaseSchema() {
  return yaml_config::MergeSchemas<dump::Dumper>(R"(
type: object
description: Base class for caching components
additionalProperties: false
properties:
    update-types:
        type: string
        description: specifies whether incremental and/or full updates will be used
        enum:
          - full-and-incremental
          - only-full
          - only-incremental
    update-interval:
        type: string
        description: (*required*) interval between Update invocations
    update-jitter:
        type: string
        description: max. amount of time by which interval may be adjusted for requests dispersal
        defaultDescription: update_interval / 10
    updates-enabled:
        type: boolean
        description: if false, cache updates are disabled (except for the first one if !first-update-fail-ok)
        defaultDescription: true
    full-update-interval:
        type: string
        description: interval between full updates
    exception-interval:
        type: string
        description: sleep interval after an unhandled exception
    first-update-fail-ok:
        type: boolean
        description: whether first update failure is non-fatal
        defaultDescription: false
    task-processor:
        type: string
        description: the name of the TaskProcessor for running DoWork
        defaultDescription: main-task-processor
    config-settings:
        type: boolean
        description: enables dynamic reconfiguration with CacheConfigSet
        defaultDescription: true
    additional-cleanup-interval:
        type: string
        description: how often to run background RCU garbage collector
        defaultDescription: 10 seconds
    is-strong-period:
        type: boolean
        description: whether to include Update execution time in update-interval
        defaultDescription: false
    has-pre-assign-check:
        type: boolean
        description: enables the check before changing the value in the cache, by default it is the check that the new value is not empty
        defaultDescription: false
    testsuite-force-periodic-update:
        type: boolean
        description: override testsuite-periodic-update-enabled in TestsuiteSupport component config
    dump:
        type: object
        description: manages dumps
        additionalProperties: false
        properties:
            first-update-mode:
                type: string
                description: specifies whether required or best-effort first update will be used
                defaultDescription: skip
            first-update-type:
                type: string
                description: specifies whether incremental and/or full first update will be used
                defaultDescription: full
)");
}

}  // namespace components::impl

USERVER_NAMESPACE_END
