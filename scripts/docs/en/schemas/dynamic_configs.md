## Dynamic configs

Here you can find the description of dynamic configs - options that can be
changed at run-time without stopping the service.

For an information on how to write a service that distributes dynamic configs
see @ref md_en_userver_tutorial_config_service.


## Adding and using your own dynamic configs

Dynamic config values could be obtained via the taxi_config::Source client that
could be retrieved from components::TaxiConfig:

@snippet components/component_sample_test.cpp  Sample user component source


To get a specific value you need a parser for it. For example, here's how you
could parse and get the `SAMPLE_INTEGER_FROM_RUNTIME_CONFIG` option:

@snippet components/component_sample_test.cpp  Sample user component runtime config source


## HTTP_CLIENT_CONNECT_THROTTLE @anchor HTTP_CLIENT_CONNECT_THROTTLE

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
```
json
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


## HTTP_CLIENT_CONNECTION_POOL_SIZE @anchor HTTP_CLIENT_CONNECTION_POOL_SIZE
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


## HTTP_CLIENT_ENFORCE_TASK_DEADLINE @anchor HTTP_CLIENT_ENFORCE_TASK_DEADLINE

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
```
json
{
  "cancel-request": false,
  "update-timeout": false
}
```

Used by components::HttpClient, affects the behavior of clients::http::Client and all the clients that use it.


## MONGO_DEFAULT_MAX_TIME_MS @anchor MONGO_DEFAULT_MAX_TIME_MS

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


## POSTGRES_DEFAULT_COMMAND_CONTROL @anchor POSTGRES_DEFAULT_COMMAND_CONTROL

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
```
json
{
  "network_timeout_ms": 750,
  "statement_timeout_ms": 500
}
```

Used by components::Postgres.


## POSTGRES_HANDLERS_COMMAND_CONTROL @anchor POSTGRES_HANDLERS_COMMAND_CONTROL

Dynamic config that controls per-handle statement and network timeouts. Overrides @ref POSTGRES_DEFAULT_COMMAND_CONTROL and
built-in timeouts from components::Postgres::kDefaultCommandControl, but may be overridden by @ref POSTGRES_QUERIES_COMMAND_CONTROL and
storages::postgres::CommandControl.

@ref POSTGRES_HANDLERS_COMMAND_CONTROL does not work if the `handlers_cmd_ctl_task_data_path_key`
and `handlers_cmd_ctl_task_data_method_key` static options of components::Postgres are not set. You can set them to
"http-handler-path" and "http-handler-method" respectively to get the values set by
server::handlers::HttpHandlerBase, or you could set some data via utils::SetTaskInheritedData using your own keys
and specify those keys in static options of components::Postgres. On other words, values of the
`handlers_cmd_ctl_task_data_path_key` and `handlers_cmd_ctl_task_data_method_key` static config options of components::Postgres
are used as utils::GetTaskInheritedData keys to get handle and method names from task local data for
@ref POSTGRES_HANDLERS_COMMAND_CONTROL.

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
```
json
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


## POSTGRES_QUERIES_COMMAND_CONTROL @anchor POSTGRES_QUERIES_COMMAND_CONTROL

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
```
json
{
  "cleanup_processed_data": {
    "network_timeout_ms": 92000,
    "statement_timeout_ms": 90000
  },
  "select_recent_users": {
    "network_timeout_ms": 70,
    "statement_timeout_ms": 30
  },
}
```

Used by components::Postgres.


## POSTGRES_CONNECTION_POOL_SETTINGS @anchor POSTGRES_CONNECTION_POOL_SETTINGS

Dynamic config that controls connection pool settings for specific service.

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
```

**Example:**
```
json
{
  "__default__": {
    "min_pool_size": 4,
    "max_pool_size": 15,
    "max_queue_size": 200
  },
  "postgresql-orders": {
    "min_pool_size": 8,
    "max_pool_size": 50,
    "max_queue_size": 200
  }
}
```

Used by components::Postgres.


## POSTGRES_STATEMENT_METRICS_SETTINGS @anchor POSTGRES_STATEMENT_METRICS_SETTINGS

Dynamic config that controls statement metrics settings for specific service.
`max_statement_metrics == 0` disables metrics export.

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

```
json
{
    "postgresql-grocery_orders": {
        "max_statement_metrics": 50
    }
}
```

Used by components::Postgres.


## USERVER_CACHES @anchor USERVER_CACHES

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
```
json
{
  "some-cache-name": {
    "full-update-interval-ms": 86400000,
    "update-interval-ms": 30000,
    "update-jitter-ms": 1000
  }
}
```

Used by all the caches derived from components::CachingComponentBase.


## USERVER_CHECK_AUTH_IN_HANDLERS @anchor USERVER_CHECK_AUTH_IN_HANDLERS

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

## USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE @anchor USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE

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

## USERVER_DUMPS @anchor USERVER_DUMPS

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
```
json
{
  "some-cache-name": {
    "dumps-enabled": true,
    "update-interval-ms": 30000,
    "min-dump-interval": 100000
  }
}
```

Used by dump::Dumper, especially by all the caches derived from components::CachingComponentBase.


## USERVER_HTTP_PROXY @anchor USERVER_HTTP_PROXY

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


## USERVER_LOG_REQUEST @anchor USERVER_LOG_REQUEST

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

## USERVER_LOG_REQUEST_HEADERS @anchor USERVER_LOG_REQUEST_HEADERS

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

## USERVER_LRU_CACHES @anchor USERVER_LRU_CACHES

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
```
json
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


## USERVER_NO_LOG_SPANS @anchor USERVER_NO_LOG_SPANS

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
```
json
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

## USERVER_RPS_CCONTROL @anchor USERVER_RPS_CCONTROL

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
```
json
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


## USERVER_RPS_CCONTROL_ENABLED @anchor USERVER_RPS_CCONTROL_ENABLED

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

## USERVER_TASK_PROCESSOR_PROFILER_DEBUG @anchor USERVER_TASK_PROCESSOR_PROFILER_DEBUG

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
```
json
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

## USERVER_TASK_PROCESSOR_QOS @anchor USERVER_TASK_PROCESSOR_QOS

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
                                        Action to perform on taks on queue overload.
                                        `cancel` - cancells the tasks
                                sensor_time_limit_us:
                                    type: integer
                                    minimum: 0
                                    description: |
                                        Wait in queue time after which the overload events for
                                        RPS congestion control are generated.
```

**Example:**
```
json
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
