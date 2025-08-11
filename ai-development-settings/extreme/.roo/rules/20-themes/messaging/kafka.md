# Kafka Integration Patterns and Implementation

## Overview

Apache Kafka integration in userver provides access to Kafka Brokers via two independent interfaces: `kafka::ProducerComponent` and `kafka::ConsumerComponent`. These components expose APIs for sending messages to Kafka topics and processing message batches from subscribed topics.

**Quality Tier**: Golden Tier

## Producer Features

### Key Characteristics
- 🚀 Parallel cooperative messages delivery reports processing (superior performance compared to single-threaded librdkafka-based clients)
- 🚀 No blocking waits in implementation (message senders suspend coroutines until delivery reports occur)
- Synchronous and asynchronous non-blocking interfaces for producing messages
- Automatic retries of transient errors
- Support of idempotent producer (exactly-once semantics)
- Sending message to concrete topic's partition

### Producer Component Configuration

```yaml
kafka-producer-name:
  delivery_timeout: 3000ms
  queue_buffering_max: 100ms
  enable_idempotence: true
  security_protocol: PLAINTEXT
  rd_kafka_custom_options:
    enable.gapless.guarantee: true
```

### Key Configuration Options
- `client_id`: Client identifier (arbitrary string, defaults to "userver")
- `delivery_timeout`: Time a produced message waits for successful delivery
- `queue_buffering_max`: Delay to wait for messages to be transmitted to broker
- `enable_idempotence`: Whether to make producer idempotent (defaults to false)
- `queue_buffering_max_messages`: Maximum number of messages waiting for delivery (defaults to 100000)
- `queue_buffering_max_kbytes`: Maximum size of messages waiting for delivery (defaults to 1048576)
- `message_max_bytes`: Maximum size of message (defaults to 1000000)
- `message_send_max_retries`: Maximum number of send request retries (defaults to 2147483647)
- `retry_backoff`: Backoff time before retrying send request (defaults to 100ms)
- `retry_backoff_max`: Backoff upper bound (defaults to 1000ms)
- `security_protocol`: Protocol used to communicate with brokers
- `sasl_mechanisms`: SASL mechanism for authentication
- `ssl_ca_location`: File or directory path to CA certificates for broker verification

## Consumer Features

### Key Characteristics
- 🚀 No blocking waits in implementation (message poller suspends coroutine until new events occur)
- Callback interface for handling message batches polled from subscribed topics
- Balanced consumer groups support
- Automatic rollback to last committed message when batch processing failed
- Partition offsets asynchronous commit

### Consumer Component Configuration

```yaml
kafka-consumer-name:
  group_id: test-group
  topics:
    - test-topic-1
    - test-topic-2
  auto_offset_reset: smallest
  max_batch_size: 10
  security_protocol: PLAINTEXT
```

### Key Configuration Options
- `client_id`: Client identifier (arbitrary string, defaults to "userver")
- `group_id`: Consumer group id (name)
- `topics`: List of topics consumer subscribes to
- `max_batch_size`: Maximum number of messages consumer waits for before calling callback (defaults to 1)
- `poll_timeout`: Maximum time consumer waits for messages before calling callback (defaults to 1s)
- `max_callback_duration`: Duration user callback must fit to avoid being kicked from consumer group (defaults to 5m)
- `restart_after_failure_delay`: Time consumer suspends execution if user-callback fails (defaults to 10s)
- `auto_offset_reset`: Action to take when there is no initial offset in offset store (defaults to "smallest")
- `env_pod_name`: Environment variable to substitute `{pod_name}` substring in `group_id`
- `security_protocol`: Protocol used to communicate with brokers
- `sasl_mechanisms`: SASL mechanism for authentication
- `ssl_ca_location`: File or directory path to CA certificates for broker verification

## Common Features

### Shared Capabilities
- Easy to configure (compared with raw Kafka clients with dozens of options)
- No message payload copying
- Support of SASL SCRAM-SHA-512 authentication and SSL transport for Broker communication
- Comprehensive logs of all events and errors
- Metrics collection
- Kafka message headers support

## Metrics

### Producer Metrics
- `kafka_producer.avg_ms_spent_time`: Average message producer latency
- `kafka_producer.messages_total`: Total number of sent messages
- `kafka_producer.messages_success`: Number of messages successfully written to partition
- `kafka_producer.messages_error`: Number of messages failed to produce
- `kafka_producer.connections_error`: Number of broker connection errors occurred

### Consumer Metrics
- `kafka_consumer.avg_ms_spent_time`: Average time between message written to partition and read by consumer
- `kafka_consumer.messages_total`: Total number of read messages
- `kafka_consumer.messages_success`: Number of successfully processed messages
- `kafka_consumer.messages_error`: Number of messages user-callback failed
- `kafka_consumer.connections_error`: Number of broker connection errors occurred

## Implementation Best Practices

### Producer Implementation
1. **Use Idempotent Producers**: Enable `enable_idempotence` for exactly-once semantics
2. **Configure Appropriate Timeouts**: Set `delivery_timeout` based on service level objectives
3. **Handle Delivery Reports**: Implement proper error handling for message delivery failures
4. **Batch Messages**: Use appropriate buffering settings to optimize throughput
5. **Monitor Metrics**: Track producer metrics for performance and error detection

### Consumer Implementation
1. **Design Efficient Callbacks**: Keep consumer callbacks lightweight and fast
2. **Handle Processing Failures**: Implement proper error handling and rollback strategies
3. **Configure Batch Sizes**: Set appropriate `max_batch_size` and `poll_timeout` values
4. **Use Consumer Groups**: Leverage balanced consumer groups for scalability
5. **Commit Offsets Carefully**: Implement proper offset commit strategies

## Testing and Development

### Unit Testing
Use `kafka::utest::KafkaCluster` for convenient unit testing with local consumers and producers instances.

### Functional Testing
The Kafka service tutorial shows how to test userver services with Kafka consumers and producers.

## Planned Enhancements

- ✅ Transfer from raw polling with timeouts to events processing (non-blocking message polling)
- ✅ testsuite Kafka support in OSS
- Support of different compression codecs (GZIP, LZ4, ZSTD, etc.)
- Support more SASL authentication mechanisms (GSSAPI, OAUTHBEARER)

## Cross-References

- [kafka::ProducerComponent](../../../../../d5/d88/classkafka_1_1ProducerComponent.html)
- [kafka::ConsumerComponent](../../../../../db/ddc/classkafka_1_1ConsumerComponent.html)
- [kafka::Producer](../../../../../da/df0/classkafka_1_1Producer.html)
- [kafka::ConsumerScope](../../../../../db/dbe/classkafka_1_1ConsumerScope.html)
- [Kafka Service Tutorial](../../../../../d9/d1c/md_en_2userver_2tutorial_2kafka__service.html)