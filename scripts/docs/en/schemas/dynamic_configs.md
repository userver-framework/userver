## Dynamic config schemas

Here you can find schemas of dynamic configs used by userver itself.
For general information on dynamic configs, see
@ref scripts/docs/en/userver/dynamic_config.md


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

Dictionary keys can be either the service **component name** (not database name!)
or `__default__`. The latter configuration is applied for every non-matching
PostgreSQL component of the service.

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


@anchor POSTGRES_TOPOLOGY_SETTINGS
## POSTGRES_TOPOLOGY_SETTINGS

Dynamic config that controls topology settings of service's PostgreSQL
components.

Dictionary keys can be either the service **component name** (not database name!)
or `__default__`. The latter configuration is applied for every non-matching
PostgreSQL component of the service.

Take note that it overrides the static configuration values of the service!

```
yaml
type: object
additionalProperties: false
properties:
    max_replication_lag_ms:
      type: integer
      minimum: 0
      description: maximum allowed replication lag. If equals 0 no replication 
      lag checks are performed
required:
  - max_replication_lag_ms
```

**Example**
```json
{
  "__default__": {
    "max_replication_lag_ms": 60000
  }
}
```

Used by components::Postgres.


@anchor POSTGRES_CONNECTION_SETTINGS
## POSTGRES_CONNECTION_SETTINGS

Dynamic config that controls settings for newly created connections of
PostgreSQL driver.

Dictionary keys can be either the service **component name** (not database name!)
or `__default__`. The latter configuration is applied for every non-matching
PostgreSQL component of the service.

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
  max-ttl-sec:
    type integer
    minimum: 1
```

**Example:**
```json
{
  "__default__": {
    "persistent-prepared-statements": true,
    "user-types-enabled": true,
    "max-prepared-cache-size": 5000,
    "ignore-unused-query-params": false,
    "recent-errors-threshold": 2,
    "max-ttl-sec": 3600
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
default: true
schema:
  type: boolean
```

Used by components::Postgres.


@anchor POSTGRES_STATEMENT_METRICS_SETTINGS
## POSTGRES_STATEMENT_METRICS_SETTINGS

Dynamic config that controls statement metrics settings for specific service.

Dictionary keys can be either the service **component name** (not database name!)
or `__default__`. The latter configuration is applied for every non-matching
PostgreSQL component of the service.

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
  },
  "__default__": {
    "max_statement_metrics": 150
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

**Example:**
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

**Example:**
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

Dictionary keys can be either the **database name** (not the component name!)
or `__default__`. The latter configuration is applied for every non-matching
Redis database/sentinel of the service.

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

**Example:**
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

Dictionary keys can be either the **database name** (not the component name!)
or `__default__`. The latter configuration is applied for every non-matching
Redis database/sentinel of the service.

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

**Example:**
```json
{
  "redis-database_name": {
    "per-shard-stats-enabled": true
  }
}
```

Used by components::Redis.


@anchor REDIS_RETRY_BUDGET_SETTINGS
## REDIS_RETRY_BUDGET_SETTINGS

Dynamic config that controls the retry budget (throttling) settings for
components::Redis.

Dictionary keys can be either the **database name** (not the component name!)
or `__default__`. The latter configuration is applied for every non-matching
Redis database/sentinel of the service.

```
yaml
type: object
additionalProperties:
  $ref: '#/definitions/BaseSettings'
definitions:
  BaseSettings:
    type: object
    additionalProperties: false
    properties:
        enabled:
          description: Enable retry budget for database
          type: boolean
        max-tokens:
          description: Number of tokens to start with
          type: number
          maximum: 1000
          minimum: 1
        token-ratio:
          description: Amount of tokens added on each successful request
          type: number
          maximum: 1
          minimum: 0.001
    required:
      - enabled
      - max-tokens
      - token-ratio
```

**Example:**
```json
{
  "__default__": {
    "max-tokens": 100,
    "token-ratio": 0.1,
    "enabled": true
  }
}
```

Used by components::Redis.

@anchor REDIS_REPLICA_MONITORING_SETTINGS
## REDIS_REPLICA_MONITORING_SETTINGS

Настройки отслеживания синхронизации реплик redis

Dynamic config that controls the monitoring settings for synchronizing replicas.

Dictionary keys can be either the **database name** (not the component name!)
or `__default__`. The latter configuration is applied for every non-matching
Redis database/sentinel of the service.

```
yaml
type: object
additionalProperties:
  $ref: '#/definitions/BaseSettings'
definitions:
  BaseSettings:
    type: object
    additionalProperties: false
    properties:
      enable-monitoring:
        description: set to `true` to turn on monitoring
        type: boolean
      forbid-requests-to-syncing-replicas:
        description: set to true to forbid requests to syncing replicas
        type: boolean
    required:
      - enable-monitoring
      - forbid-requests-to-syncing-replicas
```

Used by components::Redis.

**Example:**
```json
{
  "__default__": {
    "enable-monitoring": false,
    "forbid-requests-to-syncing-replicas": false
  }
}
```

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

**Example:**
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

**Example:**
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
            updates-enabled:
                type: boolean
                default: true
            exception-interval-ms:
                type: integer
                minimum: 0
            alert-on-failing-to-update-times:
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
    "update-jitter-ms": 1000,
    "alert-on-failing-to-update-times": 2
  }
}
```

Used by all the caches derived from components::CachingComponentBase.


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

@anchor USERVER_DEADLINE_PROPAGATION_ENABLED
## USERVER_DEADLINE_PROPAGATION_ENABLED

When `false`, disables deadline propagation in the service. This includes:

- reading the task-inherited deadline from HTTP headers and gRPC metadata;
- interrupting operations when deadline expires;
- propagating the deadline to downstream services and databases.

```
yaml
schema:
    type: boolean
```

**Example:**
```
true
```

Used by components::Server and ugrpc::server::ServerComponent.

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
Log locations are defined as path prefix from the Arcadia root ("taxi/uservices/services/").
Location of file may be followed by `:[line index]` to specify 1 exact log in that file.

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
            description: Locations of logs to turn on. This option is deprecated, consider using "force-enabled-level" instead.
            items:
                type: string

        force-disabled:
            type: array
            description: Locations of logs to turn off, logs with level WARNING and higher will not be affected.
                This option is deprecated, consider using "force-disabled-level" instead.
            items:
                type: string

        force-enabled-level:
            type: object
            description: |
                Locations of logs to turn on with level equal or higher to given.
                For example, to turn on all logs with level WARNING or higher in "userver/grpc",
                all logs with level DEBUG or higher in file "userver/core/src/server/server.cpp"
                and 1 log (since exact line is specified) with level TRACE in file "userver/core/src/server/http/http_request_parser.cpp":
                    "force-enabled-level": {
                        "taxi/uservices/userver/grpc": "WARNING",
                        "taxi/uservices/userver/core/src/server/server.cpp": "DEBUG",
                        "taxi/uservices/userver/core/src/server/http/http_request_parser.cpp:128": "TRACE"
                    }
            additionalProperties:
                type: string

        force-disabled-level:
            type: object
            description: |
                Locations of logs to turn off with level equal or lower to given.
                For example, to turn off all logs with level ERROR or lower in "userver/grpc",
                all logs with level INFO or lower in file "userver/core/src/server/server.cpp"
                and 1 log (since exact line is specified) with level TRACE in file "userver/core/src/server/http/http_request_parser.cpp":
                    "force-disabled-level": {
                        "taxi/uservices/userver/grpc": "ERROR",
                        "taxi/uservices/userver/core/src/server/server.cpp": "INFO",
                        "taxi/uservices/userver/core/src/server/http/http_request_parser.cpp:128": "TRACE"
                    }
            additionalProperties:
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

Dynamic config for mapping extension files with HTTP header content type.

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
⇦ @ref scripts/docs/en/userver/dynamic_config.md | @ref scripts/docs/en/userver/log_level_running_service.md ⇨
@htmlonly </div> @endhtmlonly
