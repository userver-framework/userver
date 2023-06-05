## Dynamic configs

Here you can find the description of dynamic configs - options that can be
changed at run-time without stopping the service.

For an information on how to write a service that distributes dynamic configs
see @ref md_en_userver_tutorial_config_service.


## Adding and using your own dynamic configs

Dynamic config values could be obtained via the dynamic_config::Source client
that could be retrieved from components::DynamicConfig:

@snippet components/component_sample_test.cpp  Sample user component source


To get a specific value you need a parser for it. For example, here's how you
could parse and get the `SAMPLE_INTEGER_FROM_RUNTIME_CONFIG` option:

@snippet components/component_sample_test.cpp  Sample user component runtime config source


@anchor HTTP_CLIENT_CONNECT_THROTTLE
## HTTP_CLIENT_CONNECT_THROTTLE

Token bucket throttling options for new connections (socket(3)).
* `*-limit` - token bucket size, set to `0` to disable the limit
* `*-per-second` - token bucket size refill speed

```
yaml
schema:
    type: object
    properties:
        http-limit:
            type: integer
            minimum: 0
        http-per-second:
            type: integer
            minimum: 0
        https-limit:
            type: integer
            minimum: 0
        https-per-second:
            type: integer
            minimum: 0
        per-host-limit:
            type: integer
            minimum: 0
        per-host-per-second:
            type: integer
            minimum: 0
    additionalProperties: false
    required:
      - http-limit
      - https-limit
      - per-host-limit
```

**Example:**
```json
{
  "http-limit": 6000,
  "http-per-second": 1500,
  "https-limit": 100,
  "https-per-second": 25,
  "per-host-limit": 3000,
  "per-host-per-second": 500
}
```

Used by components::HttpClient, affects the behavior of clients::http::Client and all the clients that use it.


@anchor HTTP_CLIENT_CONNECTION_POOL_SIZE
## HTTP_CLIENT_CONNECTION_POOL_SIZE
Open connections pool size for curl (CURLMOPT_MAXCONNECTS). `-1` means
"detect by multiplying easy handles count on 4".

```
schema:
    type: integer
```


**Example:**
```
5000
```

Used by components::HttpClient, affects the behavior of clients::http::Client and all the clients that use it.


@anchor HTTP_CLIENT_ENFORCE_TASK_DEADLINE
## HTTP_CLIENT_ENFORCE_TASK_DEADLINE

Dynamic config that controls task deadline across multiple services.

For example, service `A` starts a task with some deadline that goes to service `B`, `B` goes into `C`, `C` goes to `D`:
A->B->C->D. With the deadlines enabled, service `C` could detect that the deadline happened and there's no need to go
to service `D`.

```
yaml
schema:
    type: object
    properties:
        cancel-request:
            type: boolean
            description: Cancel the request if deadline happened.
        update-timeout:
            type: boolean
            description: Adjust the timeout before sending it to the next service.
    additionalProperties: false
    required:
      - cancel-request
      - update-timeout
```

**Example:**
```json
{
  "cancel-request": false,
  "update-timeout": false
}
```

Used by components::HttpClient, affects the behavior of clients::http::Client and all the clients that use it.


@anchor MONGO_DEFAULT_MAX_TIME_MS
## MONGO_DEFAULT_MAX_TIME_MS

Dynamic config that controls default $maxTimeMS for mongo requests (0 - disables default timeout).

```
yaml
schema:
    type: integer
    minimum: 0
```

**Example:**
```
200
```

Used by components::Mongo, components::MultiMongo.


@anchor MONGO_DEADLINE_PROPAGATION_ENABLED_V2
## MONGO_DEADLINE_PROPAGATION_ENABLED_V2

Dynamic config that controls whether task-inherited deadline is accounted for
while executing mongodb queries.

```
yaml
schema:
    type: boolean
```

**Example:**
```json
false
```

Used by components::Mongo, components::MultiMongo.


@anchor POSTGRES_DEFAULT_COMMAND_CONTROL
## POSTGRES_DEFAULT_COMMAND_CONTROL

Dynamic config that controls default network and statement timeouts. Overrides the built-in timeouts from components::Postgres::kDefaultCommandControl,
but could be overridden by @ref POSTGRES_HANDLERS_COMMAND_CONTROL, @ref POSTGRES_QUERIES_COMMAND_CONTROL and storages::postgres::CommandControl.

```
yaml
type: object
additionalProperties: false
properties:
  network_timeout_ms:
    type: integer
    minimum: 1
  statement_timeout_ms:
    type: integer
    minimum: 1
```

**Example:**
```json
{
  "network_timeout_ms": 750,
  "statement_timeout_ms": 500
}
```

Used by components::Postgres.


@anchor POSTGRES_HANDLERS_COMMAND_CONTROL
## POSTGRES_HANDLERS_COMMAND_CONTROL

Dynamic config that controls per-handle statement and network timeouts. Overrides @ref POSTGRES_DEFAULT_COMMAND_CONTROL and
built-in timeouts from components::Postgres::kDefaultCommandControl, but may be overridden by @ref POSTGRES_QUERIES_COMMAND_CONTROL and
storages::postgres::CommandControl.

```
yaml
type: object
additionalProperties:
  $ref: "#/definitions/CommandControlByMethodMap"
definitions:
  CommandControlByMethodMap:
    type: object
    additionalProperties:
      $ref: "#/definitions/CommandControl"
  CommandControl:
    type: object
    additionalProperties: false
    properties:
      network_timeout_ms:
        type: integer
        minimum: 1
      statement_timeout_ms:
        type: integer
        minimum: 1
```

**Example:**
```json
{
  "/v2/rules/create": {
    "POST": {
      "network_timeout_ms": 500,
      "statement_timeout_ms": 250
    }
  },
  "/v2/rules/select": {
    "POST": {
      "network_timeout_ms": 70,
      "statement_timeout_ms": 30
    }
  }
}
```

Used by components::Postgres.


@anchor POSTGRES_QUERIES_COMMAND_CONTROL
## POSTGRES_QUERIES_COMMAND_CONTROL

Dynamic config that controls per-query/per-transaction statement and network timeouts, if those were not explicitly set via
storages::postgres::CommandControl. Overrides the @ref POSTGRES_HANDLERS_COMMAND_CONTROL, @ref POSTGRES_QUERIES_COMMAND_CONTROL,
@ref POSTGRES_DEFAULT_COMMAND_CONTROL, and built-in timeouts from components::Postgres::kDefaultCommandControl.

Transaction timeouts in POSTGRES_QUERIES_COMMAND_CONTROL override the per-query timeouts in POSTGRES_QUERIES_COMMAND_CONTROL,
so the latter are ignored if transaction timeouts are set.

```
yaml
type: object
additionalProperties:
  $ref: "#/definitions/CommandControl"
definitions:
  CommandControl:
    type: object
    additionalProperties: false
    properties:
      network_timeout_ms:
        type: integer
        minimum: 1
      statement_timeout_ms:
        type: integer
        minimum: 1
```

**Example:**
```json
{
  "cleanup_processed_data": {
    "network_timeout_ms": 92000,
    "statement_timeout_ms": 90000
  },
  "select_recent_users": {
    "network_timeout_ms": 70,
    "statement_timeout_ms": 30
  }
}
```

Used by components::Postgres.


@anchor POSTGRES_CONNECTION_POOL_SETTINGS
## POSTGRES_CONNECTION_POOL_SETTINGS

Dynamic config that controls connection pool settings of PostgreSQL driver.

Take note that it overrides the static configuration values of the service!

```
yaml
type: object
additionalProperties:
  $ref: "#/definitions/PoolSettings"
definitions:
  PoolSettings:
    type: object
    additionalProperties: false
    properties:
      min_pool_size:
        type: integer
        minimum: 0
      max_pool_size:
        type: integer
        minimum: 1
      max_queue_size:
        type: integer
        minimum: 1
      connecting_limit:
        type: integer
        minimum: 0
    required:
      - min_pool_size
      - max_pool_size
      - max_queue_size
```

**Example:**
```json
{
  "__default__": {
    "min_pool_size": 4,
    "max_pool_size": 15,
    "max_queue_size": 200,
    "connecting_limit": 10
  },
  "postgresql-orders": {
    "min_pool_size": 8,
    "max_pool_size": 50,
    "max_queue_size": 200,
    "connecting_limit": 8
  }
}
```

Used by components::Postgres.


@anchor POSTGRES_CONNECTION_SETTINGS
## POSTGRES_CONNECTION_SETTINGS

Dynamic config that controls settings for newly created connections of
PostgreSQL driver.

Take note that it overrides the static configuration values of the service!

```
yaml
type: object
additionalProperties: false
properties:
  persistent-prepared-statements:
    type: boolean
    default: true
  user-types-enabled:
    type: boolean
    default: true
  max-prepared-cache-size:
    type: integer
    minimum: 1
    default: 5000
  recent-errors-threshold:
    type: integer
    minimum: 1
  ignore-unused-query-params:
    type: boolean
    default: false
```

**Example:**
```json
{
  "__default__": {
    "persistent-prepared-statements": true,
    "user-types-enabled": true,
    "max-prepared-cache-size": 5000,
    "ignore-unused-query-params": false,
    "recent-errors-threshold": 2
  }
}
```

Used by components::Postgres.

@anchor POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED
## POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED

Dynamic config that enables connlimit_mode: auto for PostgreSQL connections.
Auto mode ignores static and dynamic max_connections configs and verifies
that the cluster services use max_connections equals to PostgreSQL server's
max_connections divided by service instance count.


```
yaml
default: false
schema:
  type: boolean
```

@anchor POSTGRES_STATEMENT_METRICS_SETTINGS
## POSTGRES_STATEMENT_METRICS_SETTINGS

Dynamic config that controls statement metrics settings for specific service.

Dictionary keys can be either the service component names or `__default__`.
The latter configuration will be applied for every PostgreSQL component of
the service.

The value of `max_statement_metrics` controls the maximum size of LRU-cache
for named statement metrics. When set to 0 (default) no metrics are being
exported.

The exported data can be found as `postgresql.statement_timings`.

```
yaml
type: object
additionalProperties:
  $ref: "#/definitions/StatementMetricsSettings"
definitions:
  StatementMetricsSettings:
    type: object
    additionalProperties: false
    properties:
      max_statement_metrics:
        type: integer
        minimum: 0
```

```json
{
  "postgresql-database_name": {
    "max_statement_metrics": 50
  }
}
```

Used by components::Postgres.


@anchor REDIS_COMMANDS_BUFFERING_SETTINGS
## REDIS_COMMANDS_BUFFERING_SETTINGS

Dynamic config that controls command buffering for specific service.

Command buffering is disabled by default.

```
yaml
type: object
additionalProperties: false
properties:
  buffering_enabled:
    type: boolean
  commands_buffering_threshold:
    type: integer
    minimum: 1
  watch_command_timer_interval_us:
    type: integer
    minimum: 0
required:
  - buffering_enabled
  - watch_command_timer_interval_us
```

```json
{
  "buffering_enabled": true,
  "commands_buffering_threshold": 10,
  "watch_command_timer_interval_us": 1000
}
```

Used by components::Redis.


@anchor REDIS_DEFAULT_COMMAND_CONTROL
## REDIS_DEFAULT_COMMAND_CONTROL

Dynamic config that overrides the default timeouts, number of retries and
server selection strategy for redis commands.

```
yaml
type: object
additionalProperties: false
properties:
  best_dc_count:
    type: integer
  max_ping_latency_ms:
    type: integer
  max_retries:
    type: integer
  strategy:
    enum:
      - default
      - every_dc
      - local_dc_conductor
      - nearest_server_ping
    type: string
  timeout_all_ms:
    type: integer
  timeout_single_ms:
    type: integer
```

```json
{
  "best_dc_count": 0,
  "max_ping_latency_ms": 0,
  "max_retries": 4,
  "strategy": "default",
  "timeout_all_ms": 2000,
  "timeout_single_ms": 500
}
```

Used by components::Redis.


@anchor REDIS_METRICS_SETTINGS
## REDIS_METRICS_SETTINGS

Dynamic config that controls the metric settings for specific service.

```
yaml
type: object
additionalProperties:
  $ref: "#/definitions/MetricsSettings"
definitions:
  MetricsSettings:
    type: object
    additionalProperties: false
    properties:
      timings-enabled:
        type: boolean
        default: true
        description: enable timings statistics
      command-timings-enabled:
        type: boolean
        default: false
        description: enable statistics for individual commands
      request-sizes-enabled:
        type: boolean
        default: false
        description: enable request sizes statistics
      reply-sizes-enabled:
        type: boolean
        default: false
        description: enable response sizes statistics
```

```json
{
  "redis-database_name": {
    "timings-enabled": true,
    "command-timings-enabled": false,
    "request-sizes-enabled": false,
    "reply-sizes-enabled": false
  }
}
```

Used by components::Redis.


@anchor REDIS_PUBSUB_METRICS_SETTINGS
## REDIS_PUBSUB_METRICS_SETTINGS

Dynamic config that controls the redis pubsub metric settings for specific service.

```
yaml
type: object
additionalProperties:
  $ref: "#/definitions/PubsubMetricsSettings"
definitions:
  PubsubMetricsSettings:
    type: object
    additionalProperties: false
    properties:
      per-shard-stats-enabled:
        type: boolean
        default: true
        description: enable collecting statistics by shard
```

```json
{
  "redis-database_name": {
    "per-shard-stats-enabled": true
  }
}
```

Used by components::Redis.


@anchor REDIS_SUBSCRIBER_DEFAULT_COMMAND_CONTROL
## REDIS_SUBSCRIBER_DEFAULT_COMMAND_CONTROL

The same as @ref REDIS_DEFAULT_COMMAND_CONTROL but for subscription clients.


```
yaml
type: object
additionalProperties: false
properties:
  timeout_single_ms:
    type: integer
    minimum: 1
  timeout_all_ms:
    type: integer
    minimum: 1
  best_dc_count:
    type: integer
    minimum: 1
  max_ping_latency_ms:
    type: integer
    minimum: 1
  strategy:
    type: string
    enum:
      - default
      - every_dc
      - local_dc_conductor
      - nearest_server_ping
```

```json
{
  "best_dc_count": 0,
  "max_ping_latency_ms": 0,
  "strategy": "default",
  "timeout_all_ms": 2000,
  "timeout_single_ms": 500
}
```

Used by components::Redis.


@anchor REDIS_SUBSCRIPTIONS_REBALANCE_MIN_INTERVAL_SECONDS
## REDIS_SUBSCRIPTIONS_REBALANCE_MIN_INTERVAL_SECONDS

Dynamic config that controls the minimal interval between redis subscription
clients rebalancing.


```
yaml
minimum: 0
type: integer
default: 30
```

Used by components::Redis.


@anchor REDIS_WAIT_CONNECTED
## REDIS_WAIT_CONNECTED

Dynamic config that controls if services will wait for connections with redis
instances.


```
yaml
type: object
additionalProperties: false
properties:
  mode:
    type: string
    enum:
      - no_wait
      - master
      - slave
      - master_or_slave
      - master_and_slave
  throw_on_fail:
    type: boolean
  timeout-ms:
    type: integer
    minimum: 1
    x-taxi-cpp-type: std::chrono::milliseconds
required:
  - mode
  - throw_on_fail
  - timeout-ms
```

```json
{
  "mode": "master_or_slave",
  "throw_on_fail": false,
  "timeout-ms": 11000
}
```

Used by components::Redis.


@anchor USERVER_CACHES
## USERVER_CACHES

Cache update dynamic parameters.

```
yaml
schema:
    type: object
    additionalProperties:
        type: object
        additionalProperties: false
        properties:
            update-interval-ms:
                type: integer
                minimum: 1
            update-jitter-ms:
                type: integer
                minimum: 0
            full-update-interval-ms:
                type: integer
                minimum: 0
        required:
          - update-interval-ms
          - update-jitter-ms
          - full-update-interval-ms
```

**Example:**
```json
{
  "some-cache-name": {
    "full-update-interval-ms": 86400000,
    "update-interval-ms": 30000,
    "update-jitter-ms": 1000
  }
}
```

Used by all the caches derived from components::CachingComponentBase.


@anchor USERVER_CHECK_AUTH_IN_HANDLERS
## USERVER_CHECK_AUTH_IN_HANDLERS

Controls whether authentication checks are performed in handlers.

```
yaml
schema:
    type: boolean
```

**Example:**
```
true
```

Used by components::Server.

@anchor USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE
## USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE

Controls whether the http request task should be cancelled when the deadline received from the client is reached.

```
yaml
schema:
    type: boolean
```

**Example:**
```
true
```

Used by components::Server.

@anchor USERVER_DUMPS
## USERVER_DUMPS

Dynamic dump configuration. If the options are set for some dump then those
options override the static configuration.

```
yaml
schema:
    type: object
    additionalProperties:
        type: object
        additionalProperties: false
        properties:
            dumps-enabled:
                type: boolean
            min-dump-interval:
                description: dump interval in milliseconds
                type: integer
                minimum: 0
```

**Example:**
```json
{
  "some-cache-name": {
    "dumps-enabled": true,
    "update-interval-ms": 30000,
    "min-dump-interval": 100000
  }
}
```

Used by dump::Dumper, especially by all the caches derived from components::CachingComponentBase.

@anchor USERVER_HANDLER_STREAM_API_ENABLED
## USERVER_HANDLER_STREAM_API_ENABLED

Sets whether Stream API should be enabled for HTTP handlers.

```
yaml
schema:
    type: boolean
```

**Example:**
```
true
```

@anchor USERVER_HTTP_PROXY
## USERVER_HTTP_PROXY

Proxy for all the HTTP and HTTPS clients. Empty string disables proxy usage.

Proxy string may be prefixed with `[scheme]://` to specify which kind of proxy is used. Schemes match the [libcurl supported ones](https://curl.se/libcurl/c/CURLOPT_PROXY.html).
A proxy host string can also embed user and password.

```
yaml
schema:
    type: string
```

**Example:**
```
localhost:8090
```

Used by components::HttpClient, affects the behavior of clients::http::Client and all the clients that use it.

@anchor USERVER_LOG_DYNAMIC_DEBUG
## USERVER_LOG_DYNAMIC_DEBUG

Logging per line and file overrides.

```
yaml
default:
    force-enabled: []
    force-disabled: []

schema:
    type: object
    additionalProperties: false
    required:
      - force-enabled
      - force-disabled
    properties:
        force-enabled:
            type: array
            description: logs to turn on
            items:
                type: string

        force-disabled:
            type: array
            description: logs to turn off
            items:
                type: string
```

Used by components::LoggingConfigurator.


@anchor USERVER_LOG_REQUEST
## USERVER_LOG_REQUEST

Controls HTTP requests and responses logging.

```
yaml
schema:
    type: boolean
```

**Example:**
```
false
```

Used by components::Server.

@anchor USERVER_LOG_REQUEST_HEADERS
## USERVER_LOG_REQUEST_HEADERS

Controls whether the logging of HTTP headers in handlers is performed.

```
yaml
schema:
    type: boolean
```

**Example:**
```
false
```

Used by components::Server.

@anchor USERVER_LRU_CACHES
## USERVER_LRU_CACHES

Dynamic config for controlling size and cache entry lifetime of the LRU based caches.

```
yaml
schema:
    type: object
    additionalProperties:
        $ref: '#/definitions/CacheSettings'
    definitions:
        CacheSettings:
            type: object
            properties:
                size:
                    type: integer
                lifetime-ms:
                    type: integer
            required:
              - size
              - lifetime-ms
            additionalProperties: false
```

**Example:**
```json
{
  "some-cache-name": {
    "lifetime-ms": 5000,
    "size": 100000
  },
  "some-other-cache-name": {
    "lifetime-ms": 5000,
    "size": 400000
  }
}
```

Used by all the caches derived from cache::LruCacheComponent.


@anchor USERVER_NO_LOG_SPANS
## USERVER_NO_LOG_SPANS

Prefixes or full names of tracing::Span instances to not log.

```
yaml
schema:
    type: object
    additionalProperties: false
    required:
      - prefixes
      - names
    properties:
        prefixes:
            type: array
            items:
                type: string
        names:
            type: array
            items:
                type: string
```

**Example:**
```json
{
  "names": [
    "mongo_find"
  ],
  "prefixes": [
    "http",
    "handler"
  ]
}
```

Used by components::LoggingConfigurator and all the logging facilities.

@anchor USERVER_RPS_CCONTROL
## USERVER_RPS_CCONTROL

Dynamic config for components::Server congestion control.

```
yaml
schema:
    type: object
    additionalProperties: false
    properties:
        min-limit:
            type: integer
            minimum: 1
            description: |
                Minimal RPS limit value.

        up-rate-percent:
            type: number
            minimum: 0
            exclusiveMinimum: true
            description: |
                On how many percents increase the RPS limit every second if not in overloaded state.

        down-rate-percent:
            type: number
            minimum: 0
            maximum: 100
            exclusiveMinimum: true
            description: |
                On how many percents decrease the RPS limit every second if in overloaded state.

        overload-on-seconds:
            type: integer
            minimum: 1
            description: |
                How many seconds overload should be higher than `up-level`,
                to switch to overload state and start limiting RPS.

        overload-off-seconds:
            type: integer
            minimum: 1
            description: |
                How many seconds overload should be higher than `down-level`,
                to switch from overload state and the RPS limit start increasing.

        up-level:
            type: integer
            minimum: 1
            description: |
                Overload level that should least for `overload-on-seconds` to switch to
                overloaded state.

        down-level:
            type: integer
            minimum: 1
            description: |
                Overload level that should least for `overload-on-seconds` to
                increase RPS limits.

        no-limit-seconds:
            type: integer
            minimum: 1
            description: |
                Seconds without overload to switch from overloaded state and
                remove all the RPS limits.

       load-limit-crit-percent:
            type: integer
            description: |
                On reaching this load percent immediately switch to overloaded state.
```

**Example:**
```json
{
  "down-level": 8,
  "down-rate-percent": 1,
  "load-limit-crit-percent": 70,
  "min-limit": 2,
  "no-limit-seconds": 120,
  "overload-off-seconds": 8,
  "overload-on-seconds": 8,
  "up-level": 2,
  "up-rate-percent": 1
}
```

Used by congestion_control::Component.


@anchor USERVER_RPS_CCONTROL_ENABLED
## USERVER_RPS_CCONTROL_ENABLED

Controls whether congestion control limiting of RPS is performed (if main task processor is overloaded,
then the server starts rejecting some requests).

```
yaml
schema:
    type: boolean
```

**Example:**
```
true
```

Used by congestion_control::Component.

@anchor USERVER_TASK_PROCESSOR_PROFILER_DEBUG
## USERVER_TASK_PROCESSOR_PROFILER_DEBUG

Dynamic config for profiling the coroutine based engine of userver.
Dictionary key names are the names of engine::TaskProcessor.

```
yaml
schema:
    type: object
    properties: {}
    additionalProperties:
        $ref: '#/definitions/TaskProcessorSettings'
    definitions:
        TaskProcessorSettings:
            type: object
            required:
              - enabled
              - execution-slice-threshold-us
            additionalProperties: false
            properties:
                enabled:
                    type: boolean
                    description: |
                        Set to `true` to turn on the profiling
                execution-slice-threshold-us:
                    type: integer
                    description: |
                        Execution threshold of a coroutine on a thread without context switch.
                        If the threshold is reached then the coroutine is logged, otherwise
                        does nothing.
                    minimum: 1
```

**Example:**
```json
{
  "fs-task-processor": {
    "enabled": false,
    "execution-slice-threshold-us": 1000000
  },
  "main-task-processor": {
    "enabled": false,
    "execution-slice-threshold-us": 2000
  }
}
```

Used by components::ManagerControllerComponent.

@anchor USERVER_TASK_PROCESSOR_QOS
## USERVER_TASK_PROCESSOR_QOS

Controls engine::TaskProcessor quality of service dynamic config.

```
yaml
schema:
    type: object
    required:
      - default-service
    additionalProperties: false
    properties:
        default-service:
            type: object
            additionalProperties: false
            required:
              - default-task-processor
            properties:
                default-task-processor:
                    type: object
                    additionalProperties: false
                    required:
                      - wait_queue_overload
                    properties:
                        wait_queue_overload:
                            type: object
                            additionalProperties: false
                            required:
                              - time_limit_us
                              - length_limit
                              - action
                            properties:
                                time_limit_us:
                                    type: integer
                                    minimum: 0
                                    description: |
                                        Wait in queue time after which the `action is applied`.
                                length_limit:
                                    type: integer
                                    minimum: 0
                                    description: |
                                        Queue size after which the `action` is applied.
                                        action
                                action:
                                    type: string
                                    enum:
                                      - ignore
                                      - cancel
                                    description: |
                                        Action to perform on tasks on queue overload.
                                        `cancel` - cancels the tasks
                                sensor_time_limit_us:
                                    type: integer
                                    minimum: 0
                                    description: |
                                        Wait in queue time after which the overload events for
                                        RPS congestion control are generated.
```

**Example:**
```json
{
  "default-service": {
    "default-task-processor": {
      "wait_queue_overload": {
        "action": "cancel",
        "length_limit": 5000,
        "sensor_time_limit_us": 12000,
        "time_limit_us": 0
      }
    }
  }
}
```

Used by components::ManagerControllerComponent.

@anchor USERVER_FILES_CONTENT_TYPE_MAP
## USERVER_FILES_CONTENT_TYPE_MAP

Dynamic config for mapping extension files with http header content type
```
yaml
schema:
    type: object
    additionalProperties:
        type: string
```

**Example:**
```json
{
  ".css": "text/css",
  ".gif": "image/gif",
  ".htm": "text/html",
  ".html": "text/html",
  ".jpeg": "image/jpeg",
  ".js": "application/javascript",
  ".json": "application/json",
  ".md": "text/markdown",
  ".png": "image/png",
  ".svg": "image/svg+xml",
  "__default__": "text/plain"
}
```

Used by server::handlers::HttpHandlerStatic

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref rabbitmq_driver | @ref md_en_userver_log_level_running_service ⇨
@htmlonly </div> @endhtmlonly
