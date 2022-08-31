# Changing the log level at runtime

If your service has a server::handlers::LogLevel configured, then you may
change the current logging level at runtime, without service restart.

> Note that writing down the debug or trace logs can significantly affect the
> performance of the service. In addition to the overhead of writing down more
> log records, C++ debug symbols may be loaded from the disk to report stack
> traces. Such IO could lead to significant and unpredictable delays in task
> processing.

For a more granular logging server::handlers::DynamicDebugLog provides a way
to enable logging per source file and line basis.

# Logging specific locations
server::handlers::DynamicDebugLog provides the following REST API:
```
GET /service/log/dynamic-debug
PUT /service/log/dynamic-debug?location=LOCATION_FROM_GET
DELETE /service/log/dynamic-debug?location=LOCATION_FROM_GET
```

Note that the server::handlers::DynamicDebugLog handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/log/dynamic-debug` handle path.

## Examples:

### Get dynamic debug locations
```
bash
$ curl -X GET 'http://127.0.0.1:1188/service/log/dynamic-debug'
core/include/userver/rcu/rcu.hpp:208	0
core/include/userver/rcu/rcu.hpp:217	0
core/include/userver/rcu/rcu.hpp:230	0
core/include/userver/rcu/rcu.hpp:239	0
core/include/userver/rcu/rcu.hpp:384	0
core/include/userver/rcu/rcu.hpp:456	0
core/include/userver/rcu/rcu.hpp:461	0
core/include/userver/rcu/rcu.hpp:464	0
core/src/cache/cache_config.cpp:151	0
core/src/cache/cache_config.cpp:194	0
...
```

### Enable dynamic debug for a location
```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log/dynamic-debug?location=core/src/cache/cache_config.cpp:151'
OK
$ curl -X GET 'http://127.0.0.1:1188/service/log/dynamic-debug'
core/include/userver/rcu/rcu.hpp:208	0
core/include/userver/rcu/rcu.hpp:217	0
core/include/userver/rcu/rcu.hpp:230	0
core/include/userver/rcu/rcu.hpp:239	0
core/include/userver/rcu/rcu.hpp:384	0
core/include/userver/rcu/rcu.hpp:456	0
core/include/userver/rcu/rcu.hpp:461	0
core/include/userver/rcu/rcu.hpp:464	0
core/src/cache/cache_config.cpp:151	1
core/src/cache/cache_config.cpp:194	0
...
```

### Enable dynamic debug for a whole file
```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log/dynamic-debug?location=core/src/cache/cache_config.cpp'
OK
$ curl -X GET 'http://127.0.0.1:1188/service/log/dynamic-debug'
core/include/userver/rcu/rcu.hpp:208	0
core/include/userver/rcu/rcu.hpp:217	0
core/include/userver/rcu/rcu.hpp:230	0
core/include/userver/rcu/rcu.hpp:239	0
core/include/userver/rcu/rcu.hpp:384	0
core/include/userver/rcu/rcu.hpp:456	0
core/include/userver/rcu/rcu.hpp:461	0
core/include/userver/rcu/rcu.hpp:464	0
core/src/cache/cache_config.cpp:151	1
core/src/cache/cache_config.cpp:194	1
...
```

### Remove dynamic debug for a whole file
```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log/dynamic-debug?location=core/include/userver/rcu/rcu.hpp'
OK
$ curl -X GET 'http://127.0.0.1:1188/service/log/dynamic-debug'
core/include/userver/rcu/rcu.hpp:208	1
core/include/userver/rcu/rcu.hpp:217	1
core/include/userver/rcu/rcu.hpp:230	1
core/include/userver/rcu/rcu.hpp:239	1
core/include/userver/rcu/rcu.hpp:384	1
core/include/userver/rcu/rcu.hpp:456	1
core/include/userver/rcu/rcu.hpp:461	1
core/include/userver/rcu/rcu.hpp:464	1
core/src/cache/cache_config.cpp:151	0
core/src/cache/cache_config.cpp:194	0
...
$ curl -X DELETE 'http://127.0.0.1:1188/service/log/dynamic-debug?location=core/include/userver/rcu/rcu.hpp'
OK
$ curl -X GET 'http://127.0.0.1:1188/service/log/dynamic-debug'
core/include/userver/rcu/rcu.hpp:208	0
core/include/userver/rcu/rcu.hpp:217	0
core/include/userver/rcu/rcu.hpp:230	0
core/include/userver/rcu/rcu.hpp:239	0
core/include/userver/rcu/rcu.hpp:384	0
core/include/userver/rcu/rcu.hpp:456	0
core/include/userver/rcu/rcu.hpp:461	0
core/include/userver/rcu/rcu.hpp:464	0
core/src/cache/cache_config.cpp:151	0
core/src/cache/cache_config.cpp:194	0
...
```

# Commands for default logger
server::handlers::LogLevel provides the following REST API:
```
GET /service/log-level/
PUT /service/log-level/{level} (possible values {level}: trace, debug, info, warning, error, critical, none)
PUT /service/log-level/reset
```
Note that the server::handlers::LogLevel handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/log-level/` handle path.

## Examples:

### Get the current log-level 

Note that `/` is important at the end, without it you may get a response from
other handler.

```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/'
{"current-log-level":"info","init-log-level":"info"}
```

### Set log-level to debug

```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log-level/debug'
{"current-log-level":"debug","init-log-level":"info"}
```

### Restore the default log-level

```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log-level/reset'
{"current-log-level":"info","init-log-level":"info"}
```

# Commands for custom loggers

```
GET /service/log-level/?logger={logger}
PUT /service/log-level/{level}?logger={logger} (possible values {level}: trace, debug, info, warning, error, critical, none)
PUT /service/log-level/reset?logger={logger}
```
Note that the server::handlers::LogLevel handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration.


## Examples:

### Get the current log-level of the access logger

```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/?logger=access' 
{"init-log-level":"info","current-log-level":"info"}
```

### Set the log level of the access logger to debug

```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/debug?logger=access' -X PUT
{"init-log-level":"info","current-log-level":"debug"}
```

### Reset the current log level of the access logger

```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/reset?logger=access' -X PUT
{"init-log-level":"info","current-log-level":"info"}
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_schemas_dynamic_configs | @ref md_en_userver_requests_in_flight ⇨
@htmlonly </div> @endhtmlonly

