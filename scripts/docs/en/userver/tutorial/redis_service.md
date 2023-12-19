## Redis service

## Before you start

Make sure that you can compile and run core tests and read a basic example @ref scripts/docs/en/userver/tutorial/hello_service.md.

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

Like in @ref scripts/docs/en/userver/tutorial/hello_service.md we create a component for
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
safe DB client. In more complex cases @ref scripts/docs/en/userver/synchronization.md "
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

Static configuration of service is quite close to the configuration from @ref scripts/docs/en/userver/tutorial/hello_service.md except for the handler and DB:

@snippet samples/redis_service/static_config.yaml Redis service sample - static config


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
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the DB, and starts the
service.

To start the service manually start the DB server and run
`./samples/redis_service/userver-samples-redis_service -c </path/to/static_config.yaml>`.

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
@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
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
* @ref samples/redis_service/CMakeLists.txt

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/mongo_service.md | @ref scripts/docs/en/userver/tutorial/auth_postgres.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/redis_service/redis_service.cpp
@example samples/redis_service/static_config.yaml
@example samples/redis_service/CMakeLists.txt
