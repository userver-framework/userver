 ## Apache Kafka

**Quality:** @ref QUALITY_TIERS "Golden Tier".

🐙 **userver** provides access to Apache Kafka Brokers via
two independent interfaces: kafka::ProducerComponent and
kafka::ConsumerComponent.

They expose APIs for sending messages to Kafka topics, suspending
coroutine until message delivery succeeded or failed, and for process message
batches from the subscribed set of topics, asynchronously polling messages
in separate task processor.

## Common Features
- Easy to configure (compared with raw Kafka clients with dozens of options);
- No message payload copying;
- Support of SASL SCRAM-SHA-512 authentication and SSL transport for Broker communication;
- Comprehensive logs of all events and errors;
- Metrics;
- Kafka message headers support;

## Producer Features
- 🚀 Parallel cooperative messages delivery reports processing (in comparison
  with all librdkafka-based Kafka clients, the performance of which rests on a
  single thread);
- 🚀 No blocking waits in implementation (message senders suspend their
  coroutines execution until delivery reports occurred);
- Synchronous and asynchronous non-blocking interfaces for producing messages;
- Automatic retries of transient errors;
- Support of idempotent producer (exactly-once semantics);
- Sending message to concrete topic's partition;

## Consumer Features
- 🚀 No blocking waits in implementation (message poller suspends the coroutine
  until new events occurred);
- Callback interface for handling message batches polled from subscribed topics;
- Balanced consumer groups support;
- Automatic rollback to last committed message when batch processing failed;
- Partition offsets asynchronous commit;

## Metrics

Producer and consumer yields same set of metrics (but with different prefixes):

| Metric name                      | Labels                    | Description                                                            |
|----------------------------------|---------------------------|------------------------------------------------------------------------|
| kafka_producer.avg_ms_spent_time | `component-name`, `topic` | Average message producer latency                                       |
| kafka_producer.messages_total    | `component-name`, `topic` | Total number of sent messages                                          |
| kafka_producer.messages_success  | `component-name`, `topic` | Number of messages successfully written to partition                   |
| kafka_producer.messages_error    | `component-name`, `topic` | Number of messages failed to produce                                   |
| kafka_producer.connections_error | `component-name`          | Number of broker connection errors occurred                            |
| kafka_consumer.avg_ms_spent_time | `component-name`, `topic` | Average time between message written to partition and read by consumer |
| kafka_consumer.messages_total    | `component-name`, `topic` | Total number of read messages                                          |
| kafka_consumer.messages_success  | `component-name`, `topic` | Number of successfully processed messages                              |
| kafka_consumer.messages_error    | `component-name`, `topic` | Number of messages user-callback failed                                |
| kafka_consumer.connections_error | `component-name`          | Number of broker connection errors occurred                            |

See @ref scripts/docs/en/userver/service_monitor.md for info on how to get the metrics.

## Planned Enhancements
- ✅ Transfer from raw polling with timeouts to events processing,
  making the message polling non-blocking and leading to better library
  scalability;
- ✅ testsuite Kafka support in OSS;
- Support of different compression codecs (GZIP, LZ4, ZSTD, etc..);
- Support more SASL authentication mechanisms (GSSAPI, OAUTHBEARER);

# Unit and functional testing
- @ref scripts/docs/en/userver/tutorial/kafka_service.md shows how to test userver services with Kafka consumers and producers;
- kafka::utest::KafkaCluster may be used for convenient unit testing with local consumers and producers instances;

## More information
- For configuration see kafka::ProducerComponent and kafka::ConsumerComponent;
- For understanding of interfaces see kafka::Producer and kafka::ConsumerScope;
- Also kafka::impl::Consumer and kafka::impl::Configuration are well documented and may be useful;

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/mysql/design_and_details.md |
@ref scripts/docs/en/userver/ydb.md ⇨
@htmlonly </div> @endhtmlonly
