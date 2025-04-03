# Logging at runtime

## Changing the log level at runtime

> Note that writing down the debug or trace logs can significantly affect the
> performance of the service. In addition to the overhead of writing down more
> log records, C++ debug symbols may be loaded from the disk to report stack
> traces. Such IO could lead to significant and unpredictable delays in task
> processing.

If your service has a server::handlers::LogLevel configured, then you can
change the current logging level at runtime, without service restart. 

### Commands for the default logger

server::handlers::LogLevel provides the following REST API:
```
GET /service/log-level/
PUT /service/log-level/{level} (possible values {level}: trace, debug, info, warning, error, critical, none)
PUT /service/log-level/reset
```
Note that the server::handlers::LogLevel handler lives at the separate
`components.server.listener-monitor` port, so you have to request them using the
`listener-monitor` credentials. See @ref scripts/docs/en/userver/tutorial/production_service.md
for more info on configuration and ideas on how to change the
`/service/log-level/` handle path.

#### Examples:

1. Get the current log-level 
Note that `/` is important at the end, without it you might get a response from a different handler.
```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/'
{"current-log-level":"info","init-log-level":"info"}
```

2. Set log-level to debug
```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log-level/debug'
{"current-log-level":"debug","init-log-level":"info"}
```

3. Restore the default log-level
```
bash
$ curl -X PUT 'http://127.0.0.1:1188/service/log-level/reset'
{"current-log-level":"info","init-log-level":"info"}
```

### Commands for custom loggers

```
GET /service/log-level/?logger={logger}
PUT /service/log-level/{level}?logger={logger} (possible values {level}: trace, debug, info, warning, error, critical, none)
PUT /service/log-level/reset?logger={logger}
```
Note that the server::handlers::LogLevel handler lives at the separate
`components.server.listener-monitor` port, so you have to request them using the
`listener-monitor` credentials. See @ref scripts/docs/en/userver/tutorial/production_service.md
for more info on configuration.


#### Examples:

1. Get the current log-level of the access logger
```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/?logger=access' 
{"init-log-level":"info","current-log-level":"info"}
```

2. Set the log level of the access logger to debug
```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/debug?logger=access' -X PUT
{"init-log-level":"info","current-log-level":"debug"}
```

3. Reset the current log level of the access logger
```
bash
$ curl 'http://127.0.0.1:1188/service/log-level/reset?logger=access' -X PUT
{"init-log-level":"info","current-log-level":"info"}
```

## Logging specific locations

For a more granular logging userver provides ways
to enable logging per source file and line basis.

### with server::handlers::DynamicDebugLog

server::handlers::DynamicDebugLog provides the following REST API:
```
GET /service/log/dynamic-debug
PUT /service/log/dynamic-debug?location=LOCATION_FROM_GET
DELETE /service/log/dynamic-debug?location=LOCATION_FROM_GET
```

Note that the server::handlers::DynamicDebugLog handler lives at the separate
`components.server.listener-monitor` port, so you have to request them using the
`listener-monitor` credentials. See @ref scripts/docs/en/userver/tutorial/production_service.md
for more info on configuration and ideas on how to change the
`/service/log/dynamic-debug` handle path.

#### Examples:

1. Get dynamic debug locations
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

2. Enable dynamic debug for a location
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

3. Enable dynamic debug for a whole file
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

4. Remove dynamic debug for a whole file
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

### with USERVER_LOG_DYNAMIC_DEBUG

Logging locations can be modified with a dynamic config schema @ref USERVER_LOG_DYNAMIC_DEBUG. 

It provides enabling and disabling logs by log level (see @ref scripts/docs/en/userver/dynamic_config.md). 

Variables `force-disabled-level` and `force-enabled-level` store lists of log locations to be disabled and enabled respectively. 

Log locations are defined as path prefixes from the root directory.

File location may be followed by :[line index] without [ ] brackets to specify 1 exact log in that file. If the :[line index] without [ ] brackets is not specified then all logs are enabled in the file.

For example, the following configuration enables: 

1) all logs with level WARNING or higher in the directory "core/include/userver/rcu",

2) all logs with level DEBUG or higher in the file "core/src/server/server.cpp"

3) one log (since exact line is specified) with level TRACE in the file "core/src/server/http/http_request_parser.cpp".

```json
{
  "force-disabled": [],
  "force-enabled": [],
  "force-enabled-level": {
     "core/include/userver/rcu": "WARNING"
     "core/src/server/server.cpp": "DEBUG",
     "core/src/server/http/http_request_parser.cpp:128": "TRACE"
  }
}
```

This configuration disables:

1) all logs with level ERROR or lower in the directory "core/include/userver/rcu",

2) all logs with level INFO or lower in the file "core/src/server/server.cpp"

3) one log (since exact line is specified) with level TRACE in the file "core/src/server/http/http_request_parser.cpp".

```json
{
  "force-disabled": [],
  "force-enabled": [],
  "force-disabled-level": {
     "core/include/userver/rcu": "ERROR",
     "core/src/server/server.cpp": "INFO",
     "core/src/server/http/http_request_parser.cpp:128": "TRACE"
  }
}
```

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/schemas/dynamic_configs.md | @ref scripts/docs/en/userver/requests_in_flight.md ⇨
@htmlonly </div> @endhtmlonly

