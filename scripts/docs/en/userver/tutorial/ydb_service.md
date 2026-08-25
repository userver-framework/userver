## YDB service


## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.

Also, it is expected that you are familiar with basic YDB notions
(tables, topics, changefeeds, consumers, etc.).


## Step by step guide

In this tutorial we will write a service that demonstrates the YDB driver.
The sample covers table queries, transactions, BSON storage, and topic
consumption from changefeeds.

The service would have the following REST API:

* HTTP POST by path `/ydb/upsert-row` inserts a single row into the `events` table;
* HTTP POST by path `/ydb/upsert-rows` inserts multiple rows in one query;
* HTTP POST by path `/ydb/upsert-2rows` inserts two rows inside a transaction;
* HTTP POST by path `/ydb/select-rows` selects rows by secondary index;
* HTTP POST by path `/ydb/bson-upserting` stores a BSON document in the `orders` table;
* HTTP POST by path `/ydb/bson-reading` reads a BSON document from the `orders` table;

Additionally, the service contains two background components that consume
changefeed messages from YDB topics.


### HTTP handler base class

Most handlers in the sample inherit from a common base class that obtains
a @ref ydb::TableClient from @ref ydb::YdbComponent.
That client is thread safe, you can use it concurrently from different
threads and tasks.

@snippet samples/ydb_service/views/base_handler.hpp  YDB service sample - base handler


### UpsertRowHandler

The simplest way to execute a YQL query is to call
@ref ydb::TableClient::ExecuteDataQuery() with a @ref ydb::Query object
and bound parameters:

@snippet samples/ydb_service/views/upsert-row/post/view.cpp  YDB service sample - upsert row

Note that you can pass queries directly as string literals, or store them
in external YQL files. See @ref scripts/docs/en/userver/sql_files.md for more
information.


### UpsertRowsHandler

For bulk inserts you can bind a list of C++ structs to a YQL parameter.
The struct must declare @ref ydb::StructMemberNames and provide a
`Parse()` function for JSON deserialization:

@snippet samples/ydb_service/views/upsert-rows/post/view.cpp  YDB service sample - upsert rows struct

Then pass the vector of structs to `ExecuteDataQuery`:

@snippet samples/ydb_service/views/upsert-rows/post/view.cpp  YDB service sample - upsert rows


### Upsert2RowsHandler

To run several queries atomically, use @ref ydb::TableClient::RetryTx().
The lambda receives a @ref ydb::TxActor and should return
@ref ydb::TxAction::kCommit on success:

@snippet samples/ydb_service/views/upsert-2rows/post/view.cpp  YDB service sample - transaction upsert


### SelectRowsHandler

Reads can specify per-query @ref ydb::OperationSettings (retries, timeout,
transaction mode) and iterate over the result cursor:

@snippet samples/ydb_service/views/select-rows/post/view.cpp  YDB service sample - select rows


### BSON handlers

YDB can store opaque binary data. The sample stores BSON documents in a
`String` column and converts them using @ref formats::bson helpers.

The reading handler obtains the table client directly from the component:

@snippet samples/ydb_service/views/bson-reading/post/view.hpp  YDB service sample - bson reading handler

@snippet samples/ydb_service/views/bson-reading/post/view.cpp  YDB service sample - bson reading

The upserting handler writes the raw request body into the table:

@snippet samples/ydb_service/views/bson-upserting/post/view.hpp  YDB service sample - bson upserting handler

@snippet samples/ydb_service/views/bson-upserting/post/view.cpp  YDB service sample - bson upserting


### Topic reader components

The sample demonstrates two ways to consume YDB topics:

1. @ref ydb::TopicClient — regular topic reader;
2. @ref ydb::FederatedTopicClient — federated topic reader.

Both components start a background task on construction that creates a read
session, processes events in a loop, and restarts the session on failure:

@snippet samples/ydb_service/components/topic_reader.cpp  YDB service sample - topic reader component

The federated variant uses `GetFederatedTopicClient()` instead:

@snippet samples/ydb_service/components/federated_topic_reader.cpp  federated topic reader

The table `records` has a changefeed configured in
@ref samples/ydb_service/ydb/migrations/0002_records_changefeed.sql.
When rows are inserted or deleted, the topic reader components receive
JSON messages with the changed data.


### Static config

Static configuration of the service is quite close to the configuration from
@ref scripts/docs/en/userver/tutorial/hello_service.md except for the YDB
component, handlers and topic readers:

@snippet samples/ydb_service/static_config.yaml  YDB service sample - static config ydb

HTTP handlers:

@snippet samples/ydb_service/static_config.yaml  YDB service sample - static config handlers

Topic reader components:

@snippet samples/ydb_service/static_config.yaml  YDB service sample - static config topic readers

There are more static options for the YDB component configuration, all of
them are described at @ref ydb::YdbComponent.

See @ref scripts/docs/en/userver/ydb.md for YDB hints and more usage samples.


### int main()

Finally, we add our components to the
components::MinimalServerComponentList(),
and start the server with static configuration.

@snippet samples/ydb_service/main.cpp  YDB service sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-ydb_service
```

The sample could be started by running
`make start-userver-samples-ydb_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files, prepares and starts YDB, and starts the
service.

To start the service manually start the YDB server and run
`./samples/ydb_service/userver-samples-ydb_service -c </path/to/static_config.yaml>`.

Now you can send a request to your service from another terminal:

```
bash
$ curl -X POST 'http://localhost:8080/ydb/upsert-row' \
    -H 'Content-Type: application/json' \
    -d '{"id":"hello","name":"world","service":"demo","channel":1}'
{}
```


### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the testsuite. To do that you have to:

* Turn on the `pytest_userver.plugins.ydb` plugin:
  @snippet samples/ydb_service/tests/conftest.py  YDB service sample - testsuite conftest

* Write the tests using the @ref pytest_userver.plugins.service_client.service_client "service_client" fixture, the
  @ref pytest_userver.plugins.ydb.ydbsupport.ydb "ydb" fixture and the
  @ref testsuite.plugins.testpoint.testpoint "testpoint" fixture:
  @snippet samples/ydb_service/tests/test_upsert.py  YDB service sample - upsert functional test
  @snippet samples/ydb_service/tests/test_select.py  YDB service sample - select functional test
  @snippet samples/ydb_service/tests/test_bson_reading.py  YDB service sample - bson reading functional test
  @snippet samples/ydb_service/tests/test_bson_upserting.py  YDB service sample - bson upserting functional test
  @snippet samples/ydb_service/tests/test_topic.py  YDB service sample - topic functional test


## Full sources

See the full example:
* @ref samples/ydb_service/main.cpp
* @ref samples/ydb_service/static_config.yaml
* @ref samples/ydb_service/CMakeLists.txt
* @ref samples/ydb_service/views/base_handler.hpp
* @ref samples/ydb_service/views/upsert-row/post/view.cpp
* @ref samples/ydb_service/views/upsert-rows/post/view.cpp
* @ref samples/ydb_service/views/upsert-2rows/post/view.cpp
* @ref samples/ydb_service/views/select-rows/post/view.cpp
* @ref samples/ydb_service/views/bson-reading/post/view.cpp
* @ref samples/ydb_service/views/bson-upserting/post/view.cpp
* @ref samples/ydb_service/components/topic_reader.cpp
* @ref samples/ydb_service/components/federated_topic_reader.cpp
* @ref samples/ydb_service/ydb/migrations/0001_orders.sql
* @ref samples/ydb_service/ydb/migrations/0002_records_changefeed.sql
* @ref samples/ydb_service/tests/conftest.py
* @ref samples/ydb_service/tests/test_upsert.py
* @ref samples/ydb_service/tests/test_select.py
* @ref samples/ydb_service/tests/test_topic.py
* @ref samples/ydb_service/tests/test_bson_reading.py
* @ref samples/ydb_service/tests/test_bson_upserting.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/kafka_service.md | @ref scripts/docs/en/userver/tutorial/ydb_topic_writer_service.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/ydb_service/main.cpp
@example samples/ydb_service/static_config.yaml
@example samples/ydb_service/CMakeLists.txt
@example samples/ydb_service/views/base_handler.hpp
@example samples/ydb_service/views/upsert-row/post/view.cpp
@example samples/ydb_service/views/upsert-rows/post/view.cpp
@example samples/ydb_service/views/upsert-2rows/post/view.cpp
@example samples/ydb_service/views/select-rows/post/view.cpp
@example samples/ydb_service/views/bson-reading/post/view.cpp
@example samples/ydb_service/views/bson-upserting/post/view.cpp
@example samples/ydb_service/components/topic_reader.cpp
@example samples/ydb_service/components/federated_topic_reader.cpp
@example samples/ydb_service/ydb/migrations/0001_orders.sql
@example samples/ydb_service/ydb/migrations/0002_records_changefeed.sql
@example samples/ydb_service/tests/conftest.py
@example samples/ydb_service/tests/test_upsert.py
@example samples/ydb_service/tests/test_select.py
@example samples/ydb_service/tests/test_topic.py
@example samples/ydb_service/tests/test_bson_reading.py
@example samples/ydb_service/tests/test_bson_upserting.py
