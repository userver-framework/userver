## YDB topic writer service


## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.

It is recommended to read @ref scripts/docs/en/userver/tutorial/ydb_service.md
first, as it covers YDB table operations and topic consumption.


## Step by step guide

In this tutorial we will write a service that publishes messages to a YDB
topic using @ref ydb::TopicWriter.
The service would have the following REST API:

* HTTP POST by path `/write` with the message body in the request body
  writes the message to the configured YDB topic.


### WriteHandler declaration

Like in @ref scripts/docs/en/userver/tutorial/hello_service.md we create a
component for handling HTTP requests.
The component holds references to @ref ydb::TopicWriterManager and
@ref ydb::TopicWriter obtained from @ref ydb::TopicWriterComponent :

@snippet samples/ydb_topic_writer_service/src/write_handler.hpp  topic writer handler decl


### WriteHandler constructor

The topic writer is looked up by name configured in static config
(`messages` in this sample):

@snippet samples/ydb_topic_writer_service/src/write_handler.cpp  topic writer handler ctor


### WriteHandler::HandleRequest

To publish a message, call @ref ydb::TopicWriter::WriteMessage().
The method returns @ref ydb::TopicWriteResult with a status that indicates
whether the message was accepted into the writer queue:

@snippet samples/ydb_topic_writer_service/src/write_handler.cpp  topic writer HandleRequest

@ref ydb::TopicWriteStatus::kResourceExhausted means the internal queue is
full and the caller should retry later.
@ref ydb::TopicWriteStatus::kOk means the message was accepted for delivery.


### Static config

Static configuration of the service is quite close to the configuration from
@ref scripts/docs/en/userver/tutorial/hello_service.md except for the YDB
component, topic writer and handler:

@snippet samples/ydb_topic_writer_service/static_config.yaml  YDB topic writer service sample - static config ydb

Topic writer settings:

@snippet samples/ydb_topic_writer_service/static_config.yaml  topic writer static config

HTTP handler:

@snippet samples/ydb_topic_writer_service/static_config.yaml  YDB topic writer service sample - static config handler

There are more static options for the topic writer component configuration,
all of them are described at @ref ydb::TopicWriterComponent.

See @ref scripts/docs/en/userver/ydb.md for YDB hints and more usage samples.


### int main()

Add the handler, @ref ydb::YdbComponent and @ref ydb::TopicWriterComponent
to the components::MinimalServerComponentList() and run utils::DaemonRun:

@snippet samples/ydb_topic_writer_service/main.cpp  YDB topic writer service sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-ydb_topic_writer_service
```

The sample could be started by running
`make start-userver-samples-ydb_topic_writer_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files, prepares and starts YDB, and starts the
service.

To start the service manually start the YDB server and run
`./samples/ydb_topic_writer_service/userver-samples-ydb_topic_writer_service -c </path/to/static_config.yaml>`.

Now you can send a request to your service from another terminal:

```
bash
$ curl -X POST 'http://localhost:8080/write' -d 'hello topic writer'
Message was written to topic
```


### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the testsuite. To do that you have to:

* Turn on the `pytest_userver.plugins.ydb` plugin and create the topic
  before each test:
  @snippet samples/ydb_topic_writer_service/testsuite/conftest.py  YDB topic writer service sample - testsuite conftest

* Write the test that sends a message via the @ref pytest_userver.plugins.service_client.service_client "service_client"
  fixture and reads it back from the topic using the
  @ref pytest_userver.plugins.ydb.ydbsupport.ydb "ydb" fixture:
  @snippet samples/ydb_topic_writer_service/testsuite/test_topic_writer.py  topic writer functional test


## Full sources

See the full example:
* @ref samples/ydb_topic_writer_service/main.cpp
* @ref samples/ydb_topic_writer_service/static_config.yaml
* @ref samples/ydb_topic_writer_service/CMakeLists.txt
* @ref samples/ydb_topic_writer_service/src/write_handler.hpp
* @ref samples/ydb_topic_writer_service/src/write_handler.cpp
* @ref samples/ydb_topic_writer_service/testsuite/conftest.py
* @ref samples/ydb_topic_writer_service/testsuite/test_topic_writer.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/ydb_service.md | @ref scripts/docs/en/userver/tutorial/auth_postgres.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/ydb_topic_writer_service/main.cpp
@example samples/ydb_topic_writer_service/static_config.yaml
@example samples/ydb_topic_writer_service/CMakeLists.txt
@example samples/ydb_topic_writer_service/src/write_handler.hpp
@example samples/ydb_topic_writer_service/src/write_handler.cpp
@example samples/ydb_topic_writer_service/testsuite/conftest.py
@example samples/ydb_topic_writer_service/testsuite/test_topic_writer.py
