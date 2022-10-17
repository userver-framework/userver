# Service Statistics and Metrics (Prometheus/Graphite/...)

If your service has a server::handlers::ServerMonitor configured, then you may
get the service statistics and metrics.


## Commands

server::handlers::ServerMonitor has a REST API. Full description could be found
at server::handlers::ServerMonitor.

The simplest way to experiment with metrics is to start a sample by running
`make start-userver-samples-production_service` from the build directory and
make some requests from another terminal window, for example
`curl 'http://localhost:8086/service/monitor?format=prometheus'`.

Note that the server::handlers::ServerMonitor handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/monitor` handle path.

@note `prefix` and `path` parameters may refuse to work if one of the functions
  from userver/utils/statistics/metadata.hpp was used on a node that forms the
  path.


## Formats

Popular metrics formats are supported, like Prometheus or Graphite. Feel free
to fill a feature request or make a PR if some your favorite format is missing.

To specify the format use `format` URL parameter.


## Examples:


### Prometheus metrics by prefix

Prefixes are matched against the metric name:
```
bash
$ curl 'http://localhost:8086/service/monitor?format=prometheus-untyped&prefix=dns'
```
```
dns_client_replies{dns_reply_source="file"} 0
dns_client_replies{dns_reply_source="cached"} 0
dns_client_replies{dns_reply_source="cached-stale"} 0
dns_client_replies{dns_reply_source="cached-failure"} 0
dns_client_replies{dns_reply_source="network"} 0
dns_client_replies{dns_reply_source="network-failure"} 0
``` 


### Graphite metrics by path

Path should math the whole metric name:
```
bash
$ curl 'http://localhost:8086/service/monitor?format=graphite&path=engine.load-ms'
```
```
engine.load-ms 160 1665765043
```


### Get all the metrics in Graphite format

The amount of metrics depends on components count, threads count,
utils::statistics::MetricTag usage and configuration options.
```
bash
$ curl http://localhost:8086/service/monitor?format=graphite | sort
```
```
cache.any.documents.parse_failures;cache_name=dynamic-config-client-updater 0 1665765186
cache.any.documents.read_count;cache_name=dynamic-config-client-updater 0 1665765186
cache.any.time.last-update-duration-ms;cache_name=dynamic-config-client-updater 3 1665765186
cache.any.time.time-from-last-successful-start-ms;cache_name=dynamic-config-client-updater 2065 1665765186
cache.any.time.time-from-last-update-start-ms;cache_name=dynamic-config-client-updater 2065 1665765186
cache.any.update.attempts_count;cache_name=dynamic-config-client-updater 35 1665765186
cache.any.update.failures_count;cache_name=dynamic-config-client-updater 0 1665765186
cache.any.update.no_changes_count;cache_name=dynamic-config-client-updater 34 1665765186
cache.current-documents-count;cache_name=dynamic-config-client-updater 17 1665765186
cache.full.documents.parse_failures;cache_name=dynamic-config-client-updater 0 1665765186
cache.full.documents.read_count;cache_name=dynamic-config-client-updater 0 1665765186
cache.full.time.last-update-duration-ms;cache_name=dynamic-config-client-updater 3 1665765186
cache.full.time.time-from-last-successful-start-ms;cache_name=dynamic-config-client-updater 51688 1665765186
cache.full.time.time-from-last-update-start-ms;cache_name=dynamic-config-client-updater 51688 1665765186
cache.full.update.attempts_count;cache_name=dynamic-config-client-updater 3 1665765186
cache.full.update.failures_count;cache_name=dynamic-config-client-updater 0 1665765186
cache.full.update.no_changes_count;cache_name=dynamic-config-client-updater 2 1665765186
cache.incremental.documents.parse_failures;cache_name=dynamic-config-client-updater 0 1665765186
cache.incremental.documents.read_count;cache_name=dynamic-config-client-updater 0 1665765186
cache.incremental.time.last-update-duration-ms;cache_name=dynamic-config-client-updater 1 1665765186
cache.incremental.time.time-from-last-successful-start-ms;cache_name=dynamic-config-client-updater 2065 1665765186
cache.incremental.time.time-from-last-update-start-ms;cache_name=dynamic-config-client-updater 2065 1665765186
cache.incremental.update.attempts_count;cache_name=dynamic-config-client-updater 32 1665765186
cache.incremental.update.failures_count;cache_name=dynamic-config-client-updater 0 1665765186
cache.incremental.update.no_changes_count;cache_name=dynamic-config-client-updater 32 1665765186
congestion-control.rps.is-custom-status-activated 0 1665765186
cpu_time_sec 2.41 1665765186
dns-client.replies;dns_reply_source=cached 0 1665765186
dns-client.replies;dns_reply_source=cached-failure 0 1665765186
dns-client.replies;dns_reply_source=cached-stale 0 1665765186
dns-client.replies;dns_reply_source=file 0 1665765186
dns-client.replies;dns_reply_source=network 0 1665765186
dns-client.replies;dns_reply_source=network-failure 0 1665765186
engine.coro-pool.coroutines.active 16 1665765186
engine.coro-pool.coroutines.total 5000 1665765186
engine.load-ms 160 1665765186
engine.task-processors.context_switch.fast;task_processor=fs-task-processor 0 1665765186
engine.task-processors.context_switch.fast;task_processor=main-task-processor 0 1665765186
engine.task-processors.context_switch.fast;task_processor=monitor-task-processor 0 1665765186
engine.task-processors.context_switch.no_overloaded;task_processor=fs-task-processor 0 1665765186
engine.task-processors.context_switch.no_overloaded;task_processor=main-task-processor 740 1665765186
engine.task-processors.context_switch.no_overloaded;task_processor=monitor-task-processor 6 1665765186
engine.task-processors.context_switch.overloaded;task_processor=fs-task-processor 0 1665765186
engine.task-processors.context_switch.overloaded;task_processor=main-task-processor 1 1665765186
engine.task-processors.context_switch.overloaded;task_processor=monitor-task-processor 0 1665765186
engine.task-processors.context_switch.slow;task_processor=fs-task-processor 34 1665765186
engine.task-processors.context_switch.slow;task_processor=main-task-processor 3978 1665765186
engine.task-processors.context_switch.slow;task_processor=monitor-task-processor 29 1665765186
engine.task-processors.context_switch.spurious_wakeups;task_processor=fs-task-processor 0 1665765186
engine.task-processors.context_switch.spurious_wakeups;task_processor=main-task-processor 0 1665765186
engine.task-processors.context_switch.spurious_wakeups;task_processor=monitor-task-processor 0 1665765186
engine.task-processors.errors;task_processor=fs-task-processor;task_processor_error=wait_queue_overload 0 1665765186
engine.task-processors.errors;task_processor=main-task-processor;task_processor_error=wait_queue_overload 4 1665765186
engine.task-processors.errors;task_processor=monitor-task-processor;task_processor_error=wait_queue_overload 0 1665765186
engine.task-processors.tasks.alive;task_processor=fs-task-processor 1 1665765186
engine.task-processors.tasks.alive;task_processor=main-task-processor 10 1665765186
engine.task-processors.tasks.alive;task_processor=monitor-task-processor 5 1665765186
engine.task-processors.tasks.cancelled;task_processor=fs-task-processor 0 1665765186
engine.task-processors.tasks.cancelled;task_processor=main-task-processor 3 1665765186
engine.task-processors.tasks.cancelled;task_processor=monitor-task-processor 3 1665765186
engine.task-processors.tasks.created;task_processor=fs-task-processor 16 1665765186
engine.task-processors.tasks.created;task_processor=main-task-processor 83 1665765186
engine.task-processors.tasks.created;task_processor=monitor-task-processor 14 1665765186
engine.task-processors.tasks.finished;task_processor=fs-task-processor 15 1665765186
engine.task-processors.tasks.finished;task_processor=main-task-processor 73 1665765186
engine.task-processors.tasks.finished;task_processor=monitor-task-processor 9 1665765186
engine.task-processors.tasks.queued;task_processor=fs-task-processor 0 1665765186
engine.task-processors.tasks.queued;task_processor=main-task-processor 0 1665765186
engine.task-processors.tasks.queued;task_processor=monitor-task-processor 1 1665765186
engine.task-processors.tasks.running;task_processor=fs-task-processor 1 1665765186
engine.task-processors.tasks.running;task_processor=main-task-processor 10 1665765186
engine.task-processors.tasks.running;task_processor=monitor-task-processor 5 1665765186
engine.task-processors.worker-threads;task_processor=fs-task-processor 2 1665765186
engine.task-processors.worker-threads;task_processor=main-task-processor 6 1665765186
engine.task-processors.worker-threads;task_processor=monitor-task-processor 1 1665765186
engine.uptime-seconds 173 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.cancelled-by-deadline 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.deadline-received 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.in-flight 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.rate-limit-reached 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.reply-codes;http_code=300 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.reply-codes;http_code=500 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.reply-codes;http_code=501 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p0 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p100 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p50 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p90 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p95 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p98 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p99 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p99_6 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.timings;percentile=p99_9 0 1665765186
http.by-fallback.implicit-http-options.by-handler.handler-implicit-http-options.handler.too-many-requests-in-flight 0 1665765186
httpclient.cancelled-by-deadline 0 1665765186
httpclient.cancelled-by-deadline;http_destination=http://localhost:37975/configs/values 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-0 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-1 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-2 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-3 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-4 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-5 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-6 0 1665765186
httpclient.cancelled-by-deadline;http_worker_id=worker-7 0 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=ok 35 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=socket-error 0 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=ssl-error 0 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=timeout 0 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_destination=http://localhost:37975/configs/values;http_error=unknown-error 0 1665765186
httpclient.errors;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_error=ok 35 1665765186
httpclient.errors;http_error=socket-error 0 1665765186
httpclient.errors;http_error=ssl-error 0 1665765186
httpclient.errors;http_error=timeout 0 1665765186
httpclient.errors;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-0;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-1;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-2;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-3;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-4;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-5;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=ok 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-6;http_error=unknown-error 0 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=host-resolution-failed 0 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=ok 35 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=socket-error 0 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=ssl-error 0 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=timeout 0 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=too-many-redirects 0 1665765186
httpclient.errors;http_worker_id=worker-7;http_error=unknown-error 0 1665765186
httpclient.event-loop-load.1min 9.097983961155036e-06 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-0 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-1 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-2 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-3 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-4 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-5 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-6 0 1665765186
httpclient.event-loop-load.1min;http_worker_id=worker-7 8.188185565039533e-05 1665765186
httpclient.last-time-to-start-us 229 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-0 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-1 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-2 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-3 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-4 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-5 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-6 0 1665765186
httpclient.last-time-to-start-us;http_worker_id=worker-7 2064 1665765186
httpclient.pending-requests 0 1665765186
httpclient.pending-requests;http_destination=http://localhost:37975/configs/values 0 1665765186
httpclient.pending-requests;http_worker_id=worker-0 0 1665765186
httpclient.pending-requests;http_worker_id=worker-1 0 1665765186
httpclient.pending-requests;http_worker_id=worker-2 0 1665765186
httpclient.pending-requests;http_worker_id=worker-3 0 1665765186
httpclient.pending-requests;http_worker_id=worker-4 0 1665765186
httpclient.pending-requests;http_worker_id=worker-5 0 1665765186
httpclient.pending-requests;http_worker_id=worker-6 0 1665765186
httpclient.pending-requests;http_worker_id=worker-7 0 1665765186
httpclient.reply-statuses;http_destination=http://localhost:37975/configs/values;http_code=200 35 1665765186
httpclient.reply-statuses;http_destination=http://localhost:37975/configs/values;http_code=300 0 1665765186
httpclient.reply-statuses;http_destination=http://localhost:37975/configs/values;http_code=500 0 1665765186
httpclient.reply-statuses;http_destination=http://localhost:37975/configs/values;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-0;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-0;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-0;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-1;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-1;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-1;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-2;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-2;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-2;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-3;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-3;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-3;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-4;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-4;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-4;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-5;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-5;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-5;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-6;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-6;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-6;http_code=501 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-7;http_code=200 35 1665765186
httpclient.reply-statuses;http_worker_id=worker-7;http_code=300 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-7;http_code=500 0 1665765186
httpclient.reply-statuses;http_worker_id=worker-7;http_code=501 0 1665765186
httpclient.retries 0 1665765186
httpclient.retries;http_destination=http://localhost:37975/configs/values 0 1665765186
httpclient.retries;http_worker_id=worker-0 0 1665765186
httpclient.retries;http_worker_id=worker-1 0 1665765186
httpclient.retries;http_worker_id=worker-2 0 1665765186
httpclient.retries;http_worker_id=worker-3 0 1665765186
httpclient.retries;http_worker_id=worker-4 0 1665765186
httpclient.retries;http_worker_id=worker-5 0 1665765186
httpclient.retries;http_worker_id=worker-6 0 1665765186
httpclient.retries;http_worker_id=worker-7 0 1665765186
httpclient.sockets.active 1 1665765186
httpclient.sockets.active;http_worker_id=worker-0 0 1665765186
httpclient.sockets.active;http_worker_id=worker-1 0 1665765186
httpclient.sockets.active;http_worker_id=worker-2 0 1665765186
httpclient.sockets.active;http_worker_id=worker-3 0 1665765186
httpclient.sockets.active;http_worker_id=worker-4 0 1665765186
httpclient.sockets.active;http_worker_id=worker-5 0 1665765186
httpclient.sockets.active;http_worker_id=worker-6 0 1665765186
httpclient.sockets.active;http_worker_id=worker-7 1 1665765186
httpclient.sockets.close 0 1665765186
httpclient.sockets.close;http_worker_id=worker-0 0 1665765186
httpclient.sockets.close;http_worker_id=worker-1 0 1665765186
httpclient.sockets.close;http_worker_id=worker-2 0 1665765186
httpclient.sockets.close;http_worker_id=worker-3 0 1665765186
httpclient.sockets.close;http_worker_id=worker-4 0 1665765186
httpclient.sockets.close;http_worker_id=worker-5 0 1665765186
httpclient.sockets.close;http_worker_id=worker-6 0 1665765186
httpclient.sockets.close;http_worker_id=worker-7 0 1665765186
httpclient.sockets.open 1 1665765186
httpclient.sockets.open;http_destination=http://localhost:37975/configs/values 1 1665765186
httpclient.sockets.open;http_worker_id=worker-0 0 1665765186
httpclient.sockets.open;http_worker_id=worker-1 0 1665765186
httpclient.sockets.open;http_worker_id=worker-2 0 1665765186
httpclient.sockets.open;http_worker_id=worker-3 0 1665765186
httpclient.sockets.open;http_worker_id=worker-4 0 1665765186
httpclient.sockets.open;http_worker_id=worker-5 0 1665765186
httpclient.sockets.open;http_worker_id=worker-6 0 1665765186
httpclient.sockets.open;http_worker_id=worker-7 1 1665765186
httpclient.sockets.throttled 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-0 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-1 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-2 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-3 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-4 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-5 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-6 0 1665765186
httpclient.sockets.throttled;http_worker_id=worker-7 0 1665765186
httpclient.timeout-updated-by-deadline 0 1665765186
httpclient.timeout-updated-by-deadline;http_destination=http://localhost:37975/configs/values 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-0 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-1 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-2 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-3 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-4 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-5 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-6 0 1665765186
httpclient.timeout-updated-by-deadline;http_worker_id=worker-7 0 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p0 1 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p100 4 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p50 3 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p90 3 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p95 4 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p98 4 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p99 4 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p99_6 4 1665765186
httpclient.timings;http_destination=http://localhost:37975/configs/values;percentile=p99_9 4 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-0;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-1;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-2;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-3;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-4;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-5;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p0 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p100 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p50 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p90 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p95 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p98 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p99 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p99_6 0 1665765186
httpclient.timings;http_worker_id=worker-6;percentile=p99_9 0 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p0 1 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p100 4 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p50 3 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p90 3 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p95 4 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p98 4 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p99 4 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p99_6 4 1665765186
httpclient.timings;http_worker_id=worker-7;percentile=p99_9 4 1665765186
httpclient.timings;percentile=p0 1 1665765186
httpclient.timings;percentile=p100 4 1665765186
httpclient.timings;percentile=p50 3 1665765186
httpclient.timings;percentile=p90 3 1665765186
httpclient.timings;percentile=p95 4 1665765186
httpclient.timings;percentile=p98 4 1665765186
httpclient.timings;percentile=p99 4 1665765186
httpclient.timings;percentile=p99_6 4 1665765186
httpclient.timings;percentile=p99_9 4 1665765186
http.handler.cancelled-by-deadline;http_path=/ping;http_handler=handler-ping 0 1665765186
http.handler.cancelled-by-deadline;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control 0 1665765186
http.handler.cancelled-by-deadline;http_path=/service/inspect-requests;http_handler=handler-inspect-requests 0 1665765186
http.handler.cancelled-by-deadline;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc 0 1665765186
http.handler.cancelled-by-deadline;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log 0 1665765186
http.handler.cancelled-by-deadline;http_path=/service/log-level/_level_;http_handler=handler-log-level 0 1665765186
http.handler.cancelled-by-deadline;http_path=/service/monitor;http_handler=handler-server-monitor 0 1665765186
http.handler.cancelled-by-deadline;http_path=/tests/_action_;http_handler=tests-control 0 1665765186
http.handler.deadline-received;http_path=/ping;http_handler=handler-ping 0 1665765186
http.handler.deadline-received;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control 0 1665765186
http.handler.deadline-received;http_path=/service/inspect-requests;http_handler=handler-inspect-requests 0 1665765186
http.handler.deadline-received;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc 0 1665765186
http.handler.deadline-received;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log 0 1665765186
http.handler.deadline-received;http_path=/service/log-level/_level_;http_handler=handler-log-level 0 1665765186
http.handler.deadline-received;http_path=/service/monitor;http_handler=handler-server-monitor 0 1665765186
http.handler.deadline-received;http_path=/tests/_action_;http_handler=tests-control 0 1665765186
http.handler.in-flight;http_path=/ping;http_handler=handler-ping 0 1665765186
http.handler.in-flight;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control 0 1665765186
http.handler.in-flight;http_path=/service/inspect-requests;http_handler=handler-inspect-requests 0 1665765186
http.handler.in-flight;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc 0 1665765186
http.handler.in-flight;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log 0 1665765186
http.handler.in-flight;http_path=/service/log-level/_level_;http_handler=handler-log-level 0 1665765186
http.handler.in-flight;http_path=/service/monitor;http_handler=handler-server-monitor 1 1665765186
http.handler.in-flight;http_path=/tests/_action_;http_handler=tests-control 0 1665765186
http.handler.rate-limit-reached;http_path=/ping;http_handler=handler-ping 0 1665765186
http.handler.rate-limit-reached;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control 0 1665765186
http.handler.rate-limit-reached;http_path=/service/inspect-requests;http_handler=handler-inspect-requests 0 1665765186
http.handler.rate-limit-reached;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc 0 1665765186
http.handler.rate-limit-reached;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log 0 1665765186
http.handler.rate-limit-reached;http_path=/service/log-level/_level_;http_handler=handler-log-level 0 1665765186
http.handler.rate-limit-reached;http_path=/service/monitor;http_handler=handler-server-monitor 0 1665765186
http.handler.rate-limit-reached;http_path=/tests/_action_;http_handler=tests-control 0 1665765186
http.handler.reply-codes;http_path=/ping;http_handler=handler-ping;http_code=200 1 1665765186
http.handler.reply-codes;http_path=/ping;http_handler=handler-ping;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/ping;http_handler=handler-ping;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/ping;http_handler=handler-ping;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/service/log-level/_level_;http_handler=handler-log-level;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/service/log-level/_level_;http_handler=handler-log-level;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/service/log-level/_level_;http_handler=handler-log-level;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/service/monitor;http_handler=handler-server-monitor;http_code=200 3 1665765186
http.handler.reply-codes;http_path=/service/monitor;http_handler=handler-server-monitor;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/service/monitor;http_handler=handler-server-monitor;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/service/monitor;http_handler=handler-server-monitor;http_code=501 0 1665765186
http.handler.reply-codes;http_path=/tests/_action_;http_handler=tests-control;http_code=300 0 1665765186
http.handler.reply-codes;http_path=/tests/_action_;http_handler=tests-control;http_code=500 0 1665765186
http.handler.reply-codes;http_path=/tests/_action_;http_handler=tests-control;http_code=501 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p0 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p100 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p50 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p90 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p95 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p98 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p99 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/ping;http_handler=handler-ping;percentile=p99_9 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p0 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p100 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p50 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p90 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p95 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p98 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p99 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control;percentile=p99_9 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p0 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p100 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p50 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p90 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p95 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p98 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p99 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/service/inspect-requests;http_handler=handler-inspect-requests;percentile=p99_9 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p0 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p100 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p50 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p90 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p95 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p98 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p99 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc;percentile=p99_9 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p0 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p100 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p50 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p90 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p95 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p98 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p99 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log;percentile=p99_9 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p0 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p100 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p50 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p90 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p95 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p98 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p99 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/service/log-level/_level_;http_handler=handler-log-level;percentile=p99_9 0 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p0 23 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p100 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p50 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p90 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p95 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p98 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p99 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p99_6 28 1665765186
http.handler.timings;http_path=/service/monitor;http_handler=handler-server-monitor;percentile=p99_9 28 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p0 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p100 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p50 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p90 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p95 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p98 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p99 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p99_6 0 1665765186
http.handler.timings;http_path=/tests/_action_;http_handler=tests-control;percentile=p99_9 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/ping;http_handler=handler-ping 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/service/dnsclient/_command_;http_handler=handler-dns-client-control 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/service/inspect-requests;http_handler=handler-inspect-requests 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/service/jemalloc/prof/_command_;http_handler=handler-jemalloc 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/service/log/dynamic-debug;http_handler=handler-dynamic-debug-log 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/service/log-level/_level_;http_handler=handler-log-level 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/service/monitor;http_handler=handler-server-monitor 0 1665765186
http.handler.too-many-requests-in-flight;http_path=/tests/_action_;http_handler=tests-control 0 1665765186
http.handler.total.cancelled-by-deadline 0 1665765186
http.handler.total.deadline-received 0 1665765186
http.handler.total.in-flight 0 1665765186
http.handler.total.rate-limit-reached 0 1665765186
http.handler.total.reply-codes;http_code=200 1 1665765186
http.handler.total.reply-codes;http_code=300 0 1665765186
http.handler.total.reply-codes;http_code=500 0 1665765186
http.handler.total.reply-codes;http_code=501 0 1665765186
http.handler.total.timings;percentile=p0 0 1665765186
http.handler.total.timings;percentile=p100 0 1665765186
http.handler.total.timings;percentile=p50 0 1665765186
http.handler.total.timings;percentile=p90 0 1665765186
http.handler.total.timings;percentile=p95 0 1665765186
http.handler.total.timings;percentile=p98 0 1665765186
http.handler.total.timings;percentile=p99 0 1665765186
http.handler.total.timings;percentile=p99_6 0 1665765186
http.handler.total.timings;percentile=p99_9 0 1665765186
http.handler.total.too-many-requests-in-flight 0 1665765186
io_read_bytes 0 1665765186
io_write_bytes 0 1665765186
major_pagefaults 0 1665765186
open_files 77 1665765186
rss_kb 75376 1665765186
server.connections.active 0 1665765186
server.connections.closed 1 1665765186
server.connections.opened 1 1665765186
server.requests.active 0 1665765186
server.requests.avg-lifetime-ms 0 1665765186
server.requests.parsing 0 1665765186
server.requests.processed 1 1665765186
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_requests_in_flight | @ref md_en_userver_memory_profile_running_service ⇨
@htmlonly </div> @endhtmlonly

