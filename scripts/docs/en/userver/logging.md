# Logging and Tracing

## Logging

Modern applications use logging a lot for debugging and diagnosing a running
production service. Usually logs are harvested, indexed and stored in a separate
service for further investigation.

The userver framework addresses modern logging requirements and provides
multiple facilities for efficient work with logs, including
@ref md_en_userver_log_level_running_service.

Below are the intruductions to main developer logging facilities.


### Log level

Macros are used for logging:

@snippet logging/log_test.cpp  Sample logging usage

The right part of the expression is calculated only if the logging level is less or equal to the macro log level.
I.e., the right part of the expression `LOG_DEBUG ()<< ...` is calculated only in the case of the `DEBUG`
or `TRACE` logging level.

Sometimes it is useful to set the logging level of a given log entry dynamically:

@snippet logging/log_test.cpp  Example set custom logging usage

### Filter to the log level

Not all logs get into the log file, but only those that are not lower than the logger's log level. The logger log level
is set in static config (see components::Logging).

The log level can be changed in the static config for a particular handle. In this case, the log level of the logger is changed only
for the request handling task of the handle and for all the subtasks:

```
yaml
  components_manager:
    components:
      handler-ping:
        log-level: WARNING
```


### Dynamic change of the logging level

The logging `level` that was set in the static config of the components::Logging
component for the entire service can be changed on the fly.
See @ref md_en_userver_log_level_running_service for more info.

### Limit log length of the requests and responses

For per-handle limiting of the request body or response data logging you can use the `request_body_size_log_limit` and `response_data_size_log_limit` static options of the handler (see server::handlers::HandlerBase). Or you could override the server::handlers::HttpHandlerBase::GetRequestBodyForLogging and server::handlers::HttpHandlerBase::GetResponseDataForLogging functions.

### Limiting the log frequency

If some line of code generates too many logs and a small number of them is enough, then `LOG_*` should be
replaced with `LOG_LIMITED_*`. For example, `LOG(lvl)` should be replaced with `LOG_LIMITED(lvl)`; `LOG_DEBUG()` should be replaced with `LOG_LIMITED_DEBUG()`, etc.

In this case, the log message is written only if the message index is a power of two. The counter is reset every second.

Typical **recommended** places to use limited logging:

- **resource unavailability**

  Examples: insufficient memory; another service is not responding.

  Explanation: in case of a problem such errors are usually reported in thousands per second; their messages do not differ.

- **incorrect use of a component or service**

  Examples: negative numbers are passed to your library in a method that accepts only positive ones; the requested
  functionality is not supported by a third-party microservice.

  Explanation: Such errors should be caught at the testing stage. However, if a miracle suddenly happened and someone
  did not test the service, then the same errors are be reported in thousands per second

Nuances:

- Each thread has separate counters, so in practice there may be a little more logs
- If the `template` function logs via `LOG_LIMITED_X`, then each specialization of the function template has a
  separate counter
- If the same function with logging via `LOG_LIMITED_X` is called in different places, then all its calls
  use the same counter

### Tags

If you want to add tags to as single log record, then you can create an object of type `logging::LogExtra`, add the necessary tags to it
and output the `LogExtra` object to the log:

@snippet logging/log_extra_test.cpp  Example using LogExtra

If the same fields must be added to each log in a code block, it is recommended
to use `tracing::Span`, which implicitly adds tags to the log.

### Stacktrace

Sometimes it is useful to write a full stacktrace to the log. Typical use case is for logging a "never should happen happened"
situation. Use `logging::LogExtra::Stacktrace()` for such cases:

@snippet logging/log_extra_test.cpp  Example using stacktrace in log

Important: getting a text representation of a stacktrace is an **expensive operation**. In addition, the stacktrace
itself increases the log record size several times. Therefore, you do not need to use a stack trace for all errors. Use it only
when it is 100% useful for diagnostics and other diagnostic tools are ineffective.

### Additional loggers

You can create any number of additional loggers. All you need is to add them to
your `logging.loggers` section of the static config (see components::Logging for an example)

After that, you can get a logger in the code like this:

```cpp
auto& logging_component =
      context.FindComponent<::components::Logging>();
  my_logger = logging_component.GetLogger("my_logger_name");
```

To write to such a logger, it is convenient to use `LOG_XXX_TO`, for example:

```cpp
LOG_INFO_TO(my_logger) << "Look, I am a new logger!";
```

Note: do not forget to configure the logrotate for your new log file!

## Tracing
The userver implements a request tracing mechanism that is compatible with the
[opentelemetry](https://opentelemetry.io/docs/) standard.

It allows you to save dependencies between tasks, 
between requests through several services, 
thereby building a trace of requests and interactions.
It can be used to identify slow query stages, bottlenecks, 
sequential queries, etc.

### tracing::Span

When processing a request, you can create a `tracking::Span` object that measures the execution time of the current code block (technically, the time between its constructor and destructor) and stores the resulting time in the log:

Example log `tracing::Span span("cache_invalidate")`:

```
tskv	timestamp=2018-12-04T14:00:35.303132	timezone=+03:00	level=INFO	module=~Impl ( userver/src/tracing/span.cpp:76 ) 	task_id=140572868354752	coro_id=140579682340544	text=	trace_id=672a081c8004409ca79d5cc05cb5e580	span_id=12ff00c63bcc46599741bab62506881c	parent_id=7a7c1c6999094d2a8e3d22bc6ecf5d70	stopwatch_name=cache_invalidate	start_timestamp=1543921235.301035	total_time=2.08675	span_ref_type=child	stopwatch_units=ms
```

Log record example for some `POST /v1/upload`  handle:

```
tskv	timestamp=2020-08-13T15:30:52.507493	level=INFO	module=~Impl ( userver/core/src/tracing/span.cpp:139 ) 	task_id=7F110B765400	thread_id=0x00007F115BDEE700	text=	stopwatch_name=http/handler-v1_upload-post	total_time=36.393694	span_ref_type=child	stopwatch_units=ms	start_timestamp=1597321852.471086	meta_type=/v1/upload	_type=response	method=POST	body={"status":"ok"}	uri=/v1/upload?provider_id=driver-metrics	http_handle_request_time=36.277501	http_serialize_response_data_time=0.003394	tags_cache_mapping_time=0.018781	find_service_time=21.702876	http_parse_request_data_time=0.053233	http_check_auth_time=0.029809	http_check_ratelimit_time=0.000118	entities_cache_mapping_time=0.01037	register_request_time=0.819509	log_to_yt_time=0.047565	save_request_result_time=1.523389	upload_queries_time=5.179371	commit_time=4.11817	link=48e0029fc25e460880529b9d300967df	parent_link=b1377a1b20384fe292fd77cb96b30121	source_service=driver-metrics	entity_type=udid	merge_policy=append	provider_name=driver-metrics	tags_count_append=3	meta_code=200	trace_id=2f6bf12265934260876a236c373b37dc	span_id=8f828566189db0d0	parent_id=fdae1985431a6a57
```

`tracing::Span` can only be created on stack. Currently, the ability to create `tracing::Span` as a member of a class whose objects can be passed between tasks is not supported.

### Tags

In addition to `trace_id`, `span_id`, `parent_id` and other tags specific to opentracing, the `tracing::Span` class can store arbitrary custom tags. To do this, Span implicitly contains LogExtra. You can add tags like this:

@snippet tracing/span_test.cpp  Example using Span tracing

Unlike simple `LogExtra`, tags from `Span` are automatically logged when using `LOG_XXX()`. If you create a `Span`, and you already have a `Span`, then `LogExtra` is copied from the old one to the new one (except for the tags added via `AddNonInheritableTag`).

### Built-in tag semantics

- `TraceId` propagates both to sub-spans within a single service, and from client to server
- `Link` propagates within a single service, but not from client to server. A new link is generated for the "root" request handling task
- `SpanId` identifies a specific span. It does not propagate
- For "root" request handling spans, there are additionally:
  - `ParentSpanId`, which is the inner-most `SpanId` of the client
  - `ParentLink`, which is the `Link` of the client

### Span hierarchy and logging

All logs in the current task are implicitly linked to the current `Span` for the user, and tags of this `Span` are added to them (trace_id, span_id, etc.). All `Span` in the current task are implicitly stacked, and if there are several similar `Span`, the last created One (i.e., located at the top of the `Span` stack of the current task) will be used for logging.

@snippet tracing/span_test.cpp  Example span hierarchy

If you want to get the current `Span` (for example, you want to write something to `LogExtra`, but do not want to create an additional `Span`), then you can use the following approach:

@snippet tracing/span_test.cpp  Example get current span

### Creating a Span

A `Span` is automatically created for each request, handled by the server::handlers::HttpHandlerBase inherited handles. `trace_id`, `parent_id` and `link` are automatically populated for the request headers (if any).

Cache handlers do not have a parent Span, so a Span without a parent is automatically created for them, with a new `trace_id` for each update. 
The situation is similar for all utils::PeriodicTask.

When creating a new task via `utils::Async`, a new `Span` is created and linked with the current `Span` of the current task.

DB drivers and the components::HttpClient automatically create a Span for each request to trace them.

### Linking service requests

The HTTP client sends the current link/span_id/trace_id values in each request to the server, they do not need to be specified.

When the HTTP server handles the request, it extracts data from the request headers and puts them in the Span.

Names of the headers:

``` 
X-YaRequestId
X-YaSpanId
X-YaTraceId
```

### Selectively disabling Span logging

Using the server dynamic config @ref USERVER_NO_LOG_SPANS, you can set names and prefixes of Span names that do not need to be logged. If the span is not logged, then the ScopeTime of this span and any custom tags attached to the span via the methods of the `Add*Tag*()` are not put into the logs.

For example, this is how you can disable logging of all Span for MongoDB (that is, all Span with `stopwatch_name` starting with `mongo`) and `Span` with `stopwatch_name=test`:

```json
{
  "names": [
      "test"
  ],
  "prefixes": [
      "mongo"
  ]
}
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_formats | @ref md_en_userver_task_processors_guide ⇨
@htmlonly </div> @endhtmlonly
