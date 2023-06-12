## Redis service

## Before you start

Make sure that you can compile and run core tests and read a basic example @ref
md_en_userver_tutorial_hello_service.

## Step by step guide

Microservices that have state often work with database to store their data and
replicate that state across instances of the microservice. In this tutorial we
will write a service that is a simple key-value storage on top of Redis
database. The service would have the following Rest API:

* HTTP POST by path `/v1/key-value` with query parameters `key` and `value`
  stores the provided key and value or `409 Conflict` if such key already exists
* HTTP GET by path `/v1/key-value` with query parameter `key` returns the value
  if it exists or `404 Not Found` if it is missing
* HTTP DELETE by path `/v1/key-value` with query parameter `key` deletes the key
  if it exists and returns number of deleted keys (cannot be more than 1, since
  keys are unique in Redis database)

### HTTP handler component

Like in @ref md_en_userver_tutorial_hello_service we create a component for
handling HTTP requests:

@snippet samples/redis_service/redis_service.cpp Redis service sample - component

Note that the component holds a storages::redis::ClientPtr - a client to the
Redis database. That client is thread safe, you can use it concurrently from
different threads and tasks.

### Initializing the database

To access the database from our new component we need to find the Redis
component and request a client to a specific cluster by its name. After that we
are ready to make requests.

@snippet samples/redis_service/redis_service.cpp Redis service sample - component constructor

### KeyValue::HandleRequestThrow

In this sample we use a single handler to deal with all the HTTP methods. The
KeyValue::HandleRequestThrow member function mostly dispatches the request to
one of the member functions that actually implement the key-value storage logic:

@snippet samples/redis_service/redis_service.cpp Redis service sample - HandleRequestThrow

@warning `Handle*` functions are invoked concurrently on the same instance of
the handler class. In this sample the KeyValue component only uses the thread
safe DB client. In more complex cases @ref md_en_userver_synchronization "
synchronization primitives" should be used or data must not be mutated.

### KeyValue::GetValue

Executing a query to the Redis database is as simple as calling the
corresponding method of storages::redis::ClientPtr.

Note that some methods return an optional result, which must be checked. Here it
can indicate a missing key value.

@snippet samples/redis_service/redis_service.cpp Redis service sample - GetValue

### KeyValue::PostValue

Here we use storages::redis::Client::SetIfNotExist() to ensure not to change
already existing keys.

@snippet samples/redis_service/redis_service.cpp Redis service sample - PostValue

### KeyValue::DeleteValue

Note that mutating queries are automatically executed on a master instance.

@snippet samples/redis_service/redis_service.cpp Redis service sample - DeleteValue

### Static config

Static configuration of service is quite close to the configuration from @ref
md_en_userver_tutorial_hello_service except for the handler and DB:

@snippet samples/redis_service/static_config.yaml Redis service sample - static config

### Dynamic config

We are not planning to get new dynamic config values in this sample. Because of
that we just write the defaults to the fallback file of
the `components::DynamicConfigFallbacks` component:
@ref samples/redis_service/dynamic_config_fallback.json

All the values are described in a separate section
@ref md_en_schemas_dynamic_configs .

A production ready service would dynamically retrieve the above options at
runtime from a configuration service. See
@ref md_en_userver_tutorial_config_service for insights on how to change the
above options on the fly, without restarting the service.

### int main()

Finally, after writing down the dynamic config values into file
at `dynamic-config-fallbacks.fallback-path`, we add our component to the
components::MinimalServerComponentList(), and start the server with static
config `kStaticConfig`.

@snippet samples/redis_service/redis_service.cpp Redis service sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-redis_service
```

The sample could be started by running
`make start-userver-samples-redis_service`. The command would invoke
@ref md_en_userver_functional_testing "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the DB, and starts the
service.

To start the service manually start the DB server and run
`./samples/redis_service/userver-samples-redis_service -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

Now you can send a request to
your service from another terminal:

```
bash
$ curl -X POST 'localhost:8088/v1/key-value?key=hello&value=world' -i
HTTP/1.1 201 Created
Date: Wed, 27 Oct 2021 16:45:13 UTC
Content-Type: text/html
X-YaSpanId: 015fb0becd2926ef
X-YaRequestId: 7830671d7dd2462ba9043db532c2b82a
Server: userver/1.0.0 (20211027123413; rv:c1879aa03)
X-YaTraceId: d7422d7bcdc9493997fc687f8be24883
Connection: keep-alive
Content-Length: 5

world
$ curl -X DELETE 'localhost:8088/v1/key-value?key=hello&value=world' -i
HTTP/1.1 200 OK
Date: Wed, 27 Oct 2021 16:46:35 UTC
Content-Type: text/html
X-YaSpanId: e83698e2ef8cc729
X-YaRequestId: ffbaacae38e64bb588affa10b928b759
Server: userver/1.0.0 (20211027123413; rv:c1879aa03)
X-YaTraceId: cd3e6acc299742739bb22c795b6ef3a7
Connection: keep-alive
Content-Length: 1

1
```


### Functional testing
@ref md_en_userver_functional_testing "Functional tests" for the service could be
implemented using the testsuite. To do that you have to:

* Prepare the pytest by importing the pytest_userver.plugins.redis plugin:
  @snippet samples/redis_service/tests/conftest.py redis setup

* Add the Redis settings info to the service environment variable:
  @snippet samples/redis_service/tests/conftest.py service_env
  The @ref pytest_userver.plugins.service_client.auto_client_deps "auto_client_deps"
  fixture already knows about the redis_store fixture, so there's no need to override
  the @ref pytest_userver.plugins.service_client.extra_client_deps "extra_client_deps"
  fixture.

* Write the test:
  @snippet samples/redis_service/tests/test_redis.py  Functional test


## Full sources

See the full example:
* @ref samples/redis_service/redis_service.cpp
* @ref samples/redis_service/static_config.yaml
* @ref samples/redis_service/dynamic_config_fallback.json
* @ref samples/redis_service/CMakeLists.txt

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_mongo_service | @ref md_en_userver_tutorial_auth_postgres ⇨
@htmlonly </div> @endhtmlonly

@example samples/redis_service/redis_service.cpp
@example samples/redis_service/static_config.yaml
@example samples/redis_service/dynamic_config_fallback.json
@example samples/redis_service/CMakeLists.txt
