## PostgreSQL service

## Before you start

Make sure that you can compile and run core tests and read a basic example @ref md_en_userver_tutorial_hello_service.

Note that there is a ready to use opensource
[pg service template](https://github.com/userver-framework/pg_service_template)
to ease the development of your userver based services that use PostgreSQL
database. The template already has a preconfigured CI, build and install
scripts, testsuite and unit-tests setups.

## Step by step guide

Microservices that have state often work with database to store their data and replicate that state across instances of the microservice.
In this tutorial we will write a service that is a simple key-value storage on top of PostgreSQL database. The service would have the following Rest API:
* HTTP POST by path '/v1/key-value' with query parameters 'key' and 'value' stores the provided key and value or return an existing value for the key
* HTTP GET by path '/v1/key-value' with query parameter 'key' returns the value if it exists
* HTTP DELETE by path '/v1/key-value' with query parameter 'key' deletes the key if it exists and returns number of deleted keys

### HTTP handler component

Like in @ref md_en_userver_tutorial_hello_service we create a component for handling HTTP requests:

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - component

Note that the component holds a storages::postgres::ClusterPtr - a client to the PostgreSQL DB. That client is thread safe, you can use it concurrently from different threads and tasks.

### Initializing the database

To access the database from our new component we need to find the PostgreSQL component and request a client to the DB. After that we may create the required tables.

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - component constructor

To create a table we just execute an SQL statement, mentioning that it should go to the master instance. After that, our component is ready to process incoming requests in the KeyValue::HandleRequestThrow function. 


### KeyValue::HandleRequestThrow

In this sample we use a single handler to deal with all the HTTP methods. The KeyValue::HandleRequestThrow member function mostly dispatches the request to one of the member functions that actually implement the key-value storage logic: 

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - HandleRequestThrow

@warning `Handle*` functions are invoked concurrently on the same instance of the handler class. In this sample the KeyValue component only uses the thread safe DB client. In more complex cases @ref md_en_userver_synchronization "synchronization primitives" should be used or data must not be mutated.


### KeyValue::GetValue

Postgres driver in userver implicitly works with prepared statements. For the first time you execute the query a prepared statement is created. Executing the query next time would result in only sending the arguments for the already created prepared statement.

You could pass strings as queries, just like we done in constructor of KeyValue. Also queries could be stored in variables along with query names and reused across the functions. Name of the query could be used in dynamic configs to set the execution timeouts (see @ref POSTGRES_QUERIES_COMMAND_CONTROL).

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - GetValue


### KeyValue::PostValue

You can start a transaction by calling storages::postgres::Cluster::Begin(). Transactions are automatically rolled back, if you do not commit them.
To execute a query in transaction, just call Execute member function of a transaction. Just like with non-transactional Execute, you can pass string or storages::postgres::Query, you could reuse the 
same query in different functions. Transactions also could be named, and those names could be used in @ref POSTGRES_QUERIES_COMMAND_CONTROL.

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - PostValue


### KeyValue::DeleteValue

Note that mutating queries should be executed on a master instance.

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - DeleteValue


### Static config

Static configuration of service is quite close to the configuration from @ref md_en_userver_tutorial_hello_service
except for the handler and DB:

@snippet samples/postgres_service/static_config.yaml  Postgres service sample - static config


### Dynamic config

We are not planning to get new dynamic config values in this sample. Because of
that we just write the defaults to the fallback file of the
`components::DynamicConfigFallbacks` component:
@ref samples/postgres_service/dynamic_config_fallback.json

All the values are described in a separate section
@ref md_en_schemas_dynamic_configs .

A production ready service would dynamically retrieve the above options at runtime from a configuration service. See
@ref md_en_userver_tutorial_config_service for insights on how to change the
above options on the fly, without restarting the service.


### int main()

Finally, after writing down the dynamic config values into file at `dynamic-config-fallbacks.fallback-path`, we
add our component to the components::MinimalServerComponentList(),
and start the server with static config `kStaticConfig`.

@snippet samples/postgres_service/postgres_service.cpp  Postgres service sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-postgres_service
```

The sample could be started by running
`make start-userver-samples-postgres_service`. The command would invoke
@ref md_en_userver_functional_testing "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the DB, and starts the
service.

To start the service manually start the DB server and run
`./samples/postgres_service/userver-samples-postgres_service -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

Now you can send a request to your service from another terminal:
```
bash
$ curl -X POST 'localhost:8085/v1/key-value?key=hello&value=world' -i
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
$ curl -X POST 'localhost:8085/v1/key-value?key=hello&value=nope' -i
HTTP/1.1 409 Conflict
Date: Wed, 27 Oct 2021 16:45:56 UTC
Content-Type: text/html
X-YaSpanId: e1e2702b87ceeede
X-YaRequestId: 4f677a7cd405418ea412fd4ec540676a
Server: userver/1.0.0 (20211027123413; rv:c1879aa03)
X-YaTraceId: 203870322f704b308c4322bd44b354ed
Connection: keep-alive
Content-Length: 5

world
$ curl -X DELETE 'localhost:8085/v1/key-value?key=hello&value=world' -i
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

* Turn on the pytest_userver.plugins.postgresql plugin and provide PostgreSQL
  schema to start the database:
  @snippet samples/postgres_service/tests/conftest.py psql prepare
  The @ref pytest_userver.plugins.service_client.auto_client_deps "auto_client_deps"
  fixture already knows about the pgsql fixture, so there's no need to override
  the @ref pytest_userver.plugins.service_client.extra_client_deps "extra_client_deps"
  fixture.

* Write the test:
  @snippet samples/postgres_service/tests/test_postgres.py  Functional test


## Full sources

See the full example:
* @ref samples/postgres_service/postgres_service.cpp
* @ref samples/postgres_service/static_config.yaml
* @ref samples/postgres_service/dynamic_config_fallback.json
* @ref samples/postgres_service/CMakeLists.txt
* @ref samples/postgres_service/tests/conftest.py
* @ref samples/postgres_service/tests/test_postgres.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_grpc_service | @ref md_en_userver_tutorial_mongo_service ⇨
@htmlonly </div> @endhtmlonly

@example samples/postgres_service/postgres_service.cpp
@example samples/postgres_service/static_config.yaml
@example samples/postgres_service/dynamic_config_fallback.json
@example samples/postgres_service/CMakeLists.txt
@example samples/postgres_service/tests/conftest.py
@example samples/postgres_service/tests/test_postgres.py
