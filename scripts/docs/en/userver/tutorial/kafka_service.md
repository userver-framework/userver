## Kafka service


## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.

Also, it is expected that you are familiar with basic Kafka notions and ideas
(brokers, topics, partitions, producers, consumers, etc.).

## Step by step guide

In this tutorial we will write a service that consists of two components:
1. HTTP component that sends given message to Kafka broker. Method is HTTP POST `/produce`;
2. Basic component that reads all messages produced to configured topics;

### HTTP handler component (producer) declaration

Like in @ref scripts/docs/en/userver/tutorial/hello_service.md we create a component for
handling HTTP requests:

@snippet samples/kafka_service/src/producer_handler.hpp  Kafka service sample - producer component declaration

Note that the component holds a reference to kafka::Producer - a client to the Kafka (its writer).
That client is thread safe, you can use it concurrently from different threads
and tasks.

To use producer in the program, include it with

@snippet samples/kafka_service/src/producer_handler.hpp  Kafka service sample - producer include

### Produce messages to topic

To send messages to topics, let us declare and implement function `kafka_sample::Produce`
that accepts a producer reference and the `kafka_sample::RequestMessage`
and uses `kafka::Producer::Send` method to schedule its send to the given topic
and asynchronously wait its delivery:

@snippet samples/kafka_service/src/produce.cpp  Kafka service sample - produce

Note that kafka::Producer::Send may throw kafka::SendException if message is not
correct or delivery timedout.

kafka::SendException::IsRetryable method says whether it makes sense to retry the request.

Also see kafka::Producer::SendAsync for more flexible message delivery scheduling.

### Produce message on HTTP request

At first, we should find a producer instance and save it in our component's field:

@snippet samples/kafka_service/src/producer_handler.cpp  Kafka service sample - producer component find

Then, implement the server::handlers::HttpHandlerJsonBase method to define each request handler:

@snippet samples/kafka_service/src/producer_handler.cpp  Kafka service sample - producer handler implementation

Here we:
- Check incomming message correctness;
- Parse request to `kafka_sample::RequestMessage`;
- Send message with `kafka_sample::Produce`
- Dispatch returned result

Great! Currently, we understand how to write messages to broker. Next, let's understand how to read them.

### Base component (consumer) declaration

Here we create a base component that just starts and does its job:

@snippet samples/kafka_service/src/consumer_handler.hpp  Kafka service sample - consumer component declaration

Note that component holds a reference to kafka::ConsumerScope - a client to the Kafka (its reader).
Consumer should be launched on the component start to asynchronously handle consumed messages.

To use consumer in the program, include it with

@snippet samples/kafka_service/src/consumer_handler.hpp  Kafka service sample - consumer include

### Consume messages from topics

We should implement a method or function that handles the messages consumed from topics.
Here, we are reading the message batch and call the testpoint `message_consumed` on each message key.
But in real service, the process may take a long time and do worthy things.

@snippet samples/kafka_service/src/consume.cpp  Kafka service sample - consume

### Consume messages in component

At first, we should find a consumer instance and save it in our component's field.
After that we start the asynchronous message processing process, passing a callback
to ConsumerScope::Start that calls the `kafka_sample::Consume` and commits
the messages offsets on success (no exception thrown in `Consume`).

@snippet samples/kafka_service/src/consumer_handler.cpp  Kafka service sample - consumer usage

Excellent! We understand how to read messages from broker, too. Finally, start the service.

### int main()

Add all created and required components and run utils::DaemonRun to start the server:

@snippet samples/kafka_service/main.cpp Kafka service sample - main

### Static config

Static configuration of service is quite close to the configuration from @ref scripts/docs/en/userver/tutorial/hello_service.md except for the handlers and producer and consumer settings.

Consumer settings are:

@snippet samples/kafka_service/static_config.yaml  Kafka service sample - consumer static config

And basic producer setting is:

@snippet samples/kafka_service/static_config.yaml  Kafka service sample - producer static config

### Secdist

Kafka clients need to know the Kafka brokers endpoints and, optionally, the connection
username and password.

Producer and Consumer read the aforementioned settings from Secdist in format:

```json
"kafka_settings": {
    "<kafka-component-name>": {
        "brokers": "<brokers comma-separated endpoint list>",
        "username": "SASL2 username (may be empty if use PLAINTEXT)",
        "password": "SASL2 password (may be empty if use PLAINTEXT)"
    }
}
```

### Build and run

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-kafka_service
```

The sample could be started by running
`make start-userver-samples-kafka_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the Kafka broker, and starts the
service.

Make sure to place secdist settings into `/etc/kafka_service/secure_data.json`
and define the `KAFKA_HOME` environment variable with the path to Kafka
binaries.

Also set the `TESTSUITE_KAFKA_CUSTOM_TOPICS` environment to create topics on server start.

Read about Kafka service and environment settings in https://yandex.github.io/yandex-taxi-testsuite/kafka/.

Final start command may look as follows:
```bash
 $ KAFKA_HOME=/etc/kafka TESTSUITE_KAFKA_CUSTOM_TOPICS=test-topic-1:1,test-topic-2:1 make start-userver-samples-kafka_service
```

To start the service manually start the Kafka broker and run
`./samples/kafka_service/userver-samples-kafka_service -c </path/to/static_config.yaml>`.

Now you can send a request to
your service from another terminal:
```bash
$ curl -X POST -i --data '{"topic": "test-topic-1", "key": "key", "payload": "msg"}' localhost:8187/produce
HTTP/1.1 200 OK
Date: Thu, 03 Oct 2024 12:54:27 UTC
Server: userver/2.5-rc (20241003123116; rv:unknown)
Content-Type: application/json; charset=utf-8
Accept-Encoding: gzip, zstd, identity
traceparent: 00-93a06ecf62ff45828985bae8c5e6e31b-6a0abcac1b7e835b-01
X-YaRequestId: bd1a8240fad04a259dab26d0aa8603a4
X-YaTraceId: 93a06ecf62ff45828985bae8c5e6e31b
X-YaSpanId: 6a0abcac1b7e835b
Connection: keep-alive
Content-Length: 39

{"message":"Message send successfully"}
```

### Unit tests

@warning There may be issues with environment settings, so read kafka::utest::KafkaCluster and [testsuite documentation](https://yandex.github.io/yandex-taxi-testsuite/kafka/) before write your own tests.

To write unittest with Kafka environment, link your project
with `userver::kafka-utest` target and call `userver_add_utest`
with adjusted environment:

@snippet samples/kafka_service/CMakeLists.txt  Kafka service sample - kafka unit test cmake

For unit testing your service logic,
it is useful to add functions that accepts
the producer or consumer and call them
both from service code and from tests.

To create producer and consumer instances
in userver unit tests you
can use kafka::utest::KafkaCluster fixture.
Inherit your testing class from it and
use `UTEST_F` macros.

@snippet samples/kafka_service/unittest/kafka_test.cpp  Kafka service sample - kafka utest include

To test `kafka_sample::Produce` we can use such technic:

@snippet samples/kafka_service/unittest/kafka_test.cpp  Kafka service sample - producer unit test

To test `kafka_sample::Consume` use kafka::utest::KafkaCluster::SendMessages and kafka::utest::KafkaCluster::ReceiveMessages:

@snippet samples/kafka_service/unittest/kafka_test.cpp  Kafka service sample - consumer unit test

### Functional tests

Yandex Taxi Testsuite framework has Kafka plugin that helps set
environment for functional service testing.
Also, userver has some custom fixtures in pytest_userver.plugins.kafka

Fixtures and environment settings settings may be included in
your `conftest.py` with:

@snippet samples/kafka_service/testsuite/conftest.py  Kafka service sample - kafka testsuite include

Also, you can redefine a service_env fixture and pass kafka_secdist
fixture to generate secdists settings for all yours consumers
and producers:

@snippet samples/kafka_service/testsuite/conftest.py  Kafka service sample - secdist

For sample service it generates:
```json
{
    "kafka_settings": {
        "kafka-producer":  {
            "brokers": "[::1]:9099",
            "username": "",
            "password": ""
        },
        "kafka-consumer":  {
            "brokers": "[::1]:9099",
            "username": "",
            "password": ""
        }
    }
}
```

Add functional tests using:

@snippet samples/kafka_service/CMakeLists.txt  Kafka service sample - kafka functional test cmake

Basic test that send 10 messages to 2 topics
and waits until each message is delivered:

@snippet samples/kafka_service/testsuite/test_kafka.py  Kafka service sample - kafka functional test example

Also, testsuite exposes [kafka_producer](https://yandex.github.io/yandex-taxi-testsuite/kafka/#kafka-producer) and [kafka_consumer](https://yandex.github.io/yandex-taxi-testsuite/kafka/#kafka-consumer) fixtures that also
can be used in the functional tests.

@see https://yandex.github.io/yandex-taxi-testsuite/kafka/#kafka-consumer

## Full sources

See the full example at:
* @ref samples/kafka_service/main.cpp
* @ref samples/kafka_service/CMakeLists.txt
* @ref samples/kafka_service/static_config.yaml
* @ref samples/kafka_service/src/producer_handler.hpp
* @ref samples/kafka_service/src/producer_handler.cpp
* @ref samples/kafka_service/src/consumer_handler.hpp
* @ref samples/kafka_service/src/consumer_handler.hpp
* @ref samples/kafka_service/src/produce.hpp
* @ref samples/kafka_service/src/produce.cpp
* @ref samples/kafka_service/src/consume.hpp
* @ref samples/kafka_service/src/consume.cpp
* @ref samples/kafka_service/testsuite/conftest.py
* @ref samples/kafka_service/testsuite/test_kafka.py
* @ref samples/kafka_service/unittest/kafka_test.cpp

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/redis_service.md | @ref scripts/docs/en/userver/tutorial/auth_postgres.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/kafka_service/main.cpp
@example samples/kafka_service/CMakeLists.txt
@example samples/kafka_service/static_config.yaml
@example samples/kafka_service/src/producer_handler.hpp
@example samples/kafka_service/src/producer_handler.cpp
@example samples/kafka_service/src/consumer_handler.hpp
@example samples/kafka_service/src/consumer_handler.hpp
@example samples/kafka_service/src/produce.hpp
@example samples/kafka_service/src/produce.cpp
@example samples/kafka_service/src/consume.hpp
@example samples/kafka_service/src/consume.cpp
@example samples/kafka_service/testsuite/conftest.py
@example samples/kafka_service/testsuite/test_kafka.py
@example samples/kafka_service/unittest/kafka_test.cpp
