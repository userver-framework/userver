#include <userver/cache/caching_component_base.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

yaml_config::Schema GetCachingComponentBaseSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
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
            enable:
                type: boolean
                description: Whether this `Dumper` should actually read and write dumps
            world-readable:
                type: boolean
                description: If true, dumps are created with access 0444, otherwise with access 0400
            format-version:
                type: integer
                description: Allows to ignore dumps written with an obsolete format-version
            max-age:
                type: string
                description: Overdue dumps are ignored
                defaultDescription: null
            max-count:
                type: integer
                description: Old dumps over the limit are removed from disk
                defaultDescription: 1
            min-interval:
                type: string
                description: "`WriteDumpAsync` calls performed in a fast succession are ignored"
                defaultDescription: 0s
            fs-task-processor:
                type: string
                description: "`TaskProcessor` for blocking disk IO"
                defaultDescription: fs-task-processor
            encrypted:
                type: boolean
                description: Whether to encrypt the dump
                defaultDescription: false
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
