 ## Apache Kafka

**Quality:** @ref QUALITY_TIERS "Silver Tier".

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

## Producer Features
- 🚀 Parallel cooperative messages delivery reports processing (in comparison with all librdkafka-based Kafka clients, the performance of which rests on a single thread);
- 🚀 No blocking waits in implementation (message senders suspend their coroutines execution until delivery reports occured);
- Synchronous and asynchronous non-blocking interfaces for producing messages;
- Automatic retries of transient errors;
- Support of idempotent producer (exactly-once semantics);
- Sending message to concrete topic's partition;

## Consumer Features
- 🚀 No blocking waits in implementation (message poller suspends the coroutine until new events occured);
- Callback interface for handling message batches polled from subscribed topics;
- Balanced consumer groups support;
- Automatic rollback to last committed message when batch processing failed;
- Partition offsets asynchronous commit;

## Planned Enhancements
- ✅ Transfer from raw polling with timeouts to events processing,
making the message polling non-blocking and leading to better library scalability;
- testsuite Kafka support in OSS;
- Support of different compression codecs (GZIP, LZ4, ZSTD, etc..);
- Support more SASL authentication mechanisms (GSSAPI, OAUTHBEARER);

## More information
- For configuration see kafka::ProducerComponent and kafka::ConsumerComponent;
- For understanding of interfaces see kafka::Producer and kafka::ConsumerScope;

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/mysql/design_and_details.md |
@ref scripts/docs/en/userver/ydb.md ⇨
@htmlonly </div> @endhtmlonly
