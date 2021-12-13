## Pre-caching data from HTTP remote

## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref md_en_userver_tutorial_hello_service.

## Step by step guide

Consider the case when you need to write an HTTP handler that greets a user.
However, the localized strings are stored at some remote HTTP server, and those
localizations rarely change. To reduce the network traffic and improve response
times it is profitable to bulk retrieve the translations from remote and cache
them in a local cache.

In this sample we show how to write and test a cache along with creating
HTTP requests. If you wish to cache data from database prefer using
@ref md_en_userver_caches "cache specializations for DB".

### Remote HTTP server description

For the purpose of this there is some remote HTTP server that has HTTP
handlers `http://translations.sample-company.org/translations/v1/bulk` and
`http://translations.sample-company.org/translations/v1/bulk-incremental`.

First handler returns all the available translations as JSON:
```
bash
curl -X POST http://localhost:8090/translations/v1/bulk -s | jq
{
  "content": {
    "hello": {
      "ru": "Привет",
      "en": "Hello"
    },
    "wellcome": {
      "ru": "Добро пожаловать",
      "en": "Wellcome"
    }
  },
  "update_time": "2021-11-01T12:00:00z"
}
```

Second handler returns all the changed translations since "last_update" time:
```
bash
$ curl -X POST http://localhost:8090/translations/v1/bulk-incremental?last_update=2021-11-01T12:00:00z -s | jq
{
  "content": {
    "hello": {
      "ru": "Приветище"
    }
  },
  "update_time": "2021-12-01T12:00:00z"
}
```

We are planning to cache those translations in a std::unordered_map:

@snippet samples/http_caching.cpp  HTTP caching sample - datatypes


### Cache component

Our cache @ref md_en_userver_component-system "component" should have the
following fields:
* Reference to HTTP client, that is required to make HTTP requests
* URL to the incremental update handle
* URL to the full update handle 
* String with last update time from remote. This string used only from the
  components::CachingComponentBase::Update overload and it is guaranteed
  @ref md_en_userver_caches "that Update is not called concurrently", so there
  is no need to protect it from concurrent access.

@snippet samples/http_caching.cpp  HTTP caching sample - component

To create a non @ref md_en_userver_lru-cache "LRU cache" cache you have to
derive from components::CachingComponentBase, call
CacheUpdateTrait::StartPeriodicUpdates() at the component constructor and
CacheUpdateTrait::StopPeriodicUpdates() at the destructor:

@snippet samples/http_caching.cpp  HTTP caching sample - constructor destructor

The constructor initializes component fields with data from static
configuration and with references to clients.

### Update

Depending on cache configuration settings the overloaded Update function is
periodically called with different options:

@snippet samples/http_caching.cpp  HTTP caching sample - update

In this sample there are full and incremental updates, implemented in
GetAllData() and GetUpdatedData() respectively.

At the end of the Update function the components::CachingComponentBase::Set()
invocation stores data as a new cache.

Update time from remote stored into
`last_update_remote_`. Clocks on different hosts are usually out of sync, so it
is better to store the remote time if possible, rather than using a local times
from `last_update` and `now` input parameters.


### Data retrieval

To make an HTTP request call clients::http::Client::CreateRequest() to get
an instance of clients::http::Request builder. Work with builder is quite
straightforward:

@snippet samples/http_caching.cpp  HTTP caching sample - GetAllData

HTTP requests for incremental update differ only in URL and query parameter
`last_update`:
 
@snippet samples/http_caching.cpp  HTTP caching sample - GetUpdatedData

Both handlers return data in the same format, so we parse both responses using
`MergeDataInto`:

@snippet samples/http_caching.cpp  HTTP caching sample - MergeDataInto


### Static configuration

To configure the new cache component provide its own options, options from
components::CachingComponentBase:

@snippet samples/http_caching.cpp  HTTP caching sample - static config cache

Options for dependent components components::HttpClient,
components::TestsuiteSupport and support handler server::handlers::TestsControl
should be provided:

@snippet samples/http_caching.cpp  HTTP caching sample - static config deps


### Dynamic configuration

Dynamic configuration is close to the basic configuration from
@ref md_en_userver_tutorial_hello_service but should have additional options
for HTTP client:

@snippet samples/http_caching.cpp  HTTP caching sample - dynamic config

All the values are described in at @ref md_en_schemas_dynamic_configs.

A production ready service would dynamically retrieve the above options at
runtime from a configuration service. See
@ref md_en_userver_tutorial_config_service for insights on how to change the
above options on the fly, without restarting the service.


### Cache component usage

Now the cache could be used just as any other component. For example, a handler
could get a reference to the cache and use it in `HandleRequestThrow`:

@snippet samples/http_caching.cpp  HTTP caching sample - GreetUser

Note that the cache is concurrency safe
@ref md_en_userver_component-system "as all the components".


### int main()

Finally, after writing down the dynamic configuration values into file at
`taxi-config-fallbacks.fallback-path`, we
add our component to the `components::MinimalServerComponentList()`,
and start the server with static configuration `kStaticConfig`.

@snippet samples/http_caching.cpp  HTTP caching sample - main

### Build

To build the sample, execute the following build steps at the userver root
directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-http_caching
```

Start the server by running `./samples/userver-samples-http_caching`. Not that
you need a running translations service with bulk handlers. You could use the
@ref md_en_userver_tutorial_mongo_service "mongo service" for that purpose.

Now you can send a request to your server from another terminal:
```
bash
$ curl -X POST http://localhost:8089/samples/greet?username=Dear+Developer  -i
HTTP/1.1 200 OK
Date: Thu, 09 Dec 2021 17:01:44 UTC
Content-Type: text/html; charset=utf-8
X-YaRequestId: 94193f99ebf94eb58252139f2e9dace4
X-YaSpanId: 4e17e02dfa7b8322
X-YaTraceId: 306d2d54fd0543c09376a5c4bb120a88
Server: userver/1.0.0 (20211209085954; rv:d05d059a3)
Connection: keep-alive
Content-Length: 61

Привет, Dear Developer! Добро пожаловать
```

### Auto testing

The server::handlers::TestsControl and components::TestsuiteSupport components
among other things provide necessary control over the caches. You can
stop/start periodic updates or even force updates of caches by HTTP requests
to the server::handlers::TestsControl handler.

For example the following JSON forces incremental update of the
`cache-http-translations` cache:
```
json
{"invalidate_caches": {
    "update_type": "incremental",
    "names": ["cache-http-translations"]
}}
```


## Full sources

See the full example at @ref samples/http_caching.cpp
@example samples/http_caching.cpp
