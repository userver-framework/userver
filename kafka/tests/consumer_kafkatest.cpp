#include <userver/kafka/utest/kafka_fixture.hpp>

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>

#include <gmock/gmock-matchers.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ConsumerTest : public kafka::utest::KafkaCluster {};

const std::string kLargeTopic1{"lt-1"};
const std::string kLargeTopic2{"lt-2"};
const std::string kBlockingTopic{"bt"};  // Must be used only in OneConsumerPartitionOffsets test

constexpr std::size_t kNumPartitionsLargeTopic{4};
constexpr std::size_t kNumPartitionsBlockingTopic{4};

}  // namespace

UTEST_F(ConsumerTest, BrokenConfiguration) {
    kafka::impl::ConsumerConfiguration consumer_configuration{};
    consumer_configuration.max_callback_duration = std::chrono::milliseconds{10};
    consumer_configuration.rd_kafka_options["session.timeout.ms"] = "10000";

    UEXPECT_THROW(MakeConsumer("kafka-consumer", {}, consumer_configuration), std::runtime_error);
}

UTEST_F(ConsumerTest, GetTopics) {
    {
        auto consumer = MakeConsumer("kafka-consumer", {});
        auto consumer_scope = consumer.MakeConsumerScope();
        ASSERT_TRUE(consumer_scope.GetTopics().empty());
    }
    {
        auto consumer = MakeConsumer("kafka-consumer", {"topic-1", "topic-2"});
        auto consumer_scope = consumer.MakeConsumerScope();
        EXPECT_THAT(consumer_scope.GetTopics(), ::testing::ElementsAre("topic-1", "topic-2"));
    }
}

UTEST_F(ConsumerTest, OneConsumerSmallTopics) {
    const auto topic1 = GenerateTopic();
    const auto topic2 = GenerateTopic();

    const std::vector<kafka::utest::Message> test_messages{
        kafka::utest::Message{.topic = topic1, .key = "key-1", .payload = "msg-1", .partition = 0},
        kafka::utest::Message{.topic = topic1, .key = "key-2", .payload = "msg-2", .partition = 0},
        kafka::utest::Message{.topic = topic2, .key = "key-3", .payload = "msg-3", .partition = 0},
        kafka::utest::Message{.topic = topic2, .key = "key-4", .payload = "msg-4", .partition = 0},
    };
    SendMessages(test_messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic1, topic2});

    const auto received_messages = ReceiveMessages(consumer, test_messages.size());

    EXPECT_THAT(received_messages, ::testing::UnorderedElementsAreArray(test_messages));
}

UTEST_F(ConsumerTest, OneConsumerSmallTopicsWithHeaders) {
    const auto topic1 = GenerateTopic();
    const auto topic2 = GenerateTopic();

    const std::vector<kafka::utest::Message> test_messages{
        kafka::utest::Message{
            .topic = topic1,
            .key = "key-1",
            .payload = "msg-1",
            .partition = 0,
            .headers = {{"key-1", "value-1"}}
        },
        kafka::utest::Message{
            .topic = topic1,
            .key = "key-2",
            .payload = "msg-2",
            .partition = 0,
            .headers = {{"key-1", "value-2"}}
        },
        kafka::utest::Message{
            .topic = topic2,
            .key = "key-3",
            .payload = "msg-3",
            .partition = 0,
            .headers = {{"key-2", "value-3"}, {"key-2", "value-4"}}
        },
        kafka::utest::Message{.topic = topic2, .key = "key-4", .payload = "msg-4", .partition = 0},
    };
    SendMessages(test_messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic1, topic2});

    const auto received_messages = ReceiveMessages(consumer, test_messages.size());

    EXPECT_THAT(received_messages, ::testing::UnorderedElementsAreArray(test_messages));
}

UTEST_F(ConsumerTest, OneConsumerLargeTopics) {
    constexpr std::size_t kMessagesCount{2 * 2 * kNumPartitionsLargeTopic};

    std::vector<kafka::utest::Message> test_messages{kMessagesCount};
    std::generate_n(test_messages.begin(), kMessagesCount, [i = 0]() mutable {
        i += 1;
        return kafka::utest::Message{
            .topic = i % 2 == 0 ? kLargeTopic1 : kLargeTopic2,
            .key = fmt::format("key-{}", i),
            .payload = fmt::format("msg-{}", i),
            .partition = i % kNumPartitionsLargeTopic,
        };
    });
    SendMessages(test_messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{kLargeTopic1, kLargeTopic2});

    const auto received_messages = ReceiveMessages(consumer, test_messages.size());

    EXPECT_THAT(received_messages, ::testing::UnorderedElementsAreArray(test_messages));
}

UTEST_F(ConsumerTest, ManyConsumersLargeTopics) {
    constexpr std::size_t kMessagesCount{2 * 2 * kNumPartitionsLargeTopic};

    std::vector<kafka::utest::Message> test_messages{kMessagesCount};
    std::generate_n(test_messages.begin(), kMessagesCount, [i = 0]() mutable {
        i += 1;
        return kafka::utest::Message{
            .topic = i % 2 == 0 ? kLargeTopic1 : kLargeTopic2,
            .key = fmt::format("key-{}", i),
            .payload = fmt::format("msg-{}", i),
            .partition = i % kNumPartitionsLargeTopic,
        };
    });
    SendMessages(test_messages);

    auto task1 = utils::Async("receive1", [this] {
        auto consumer = MakeConsumer(
            "kafka-consumer-first",
            /*topics=*/{kLargeTopic1, kLargeTopic2}
        );
        return ReceiveMessages(consumer, kMessagesCount / 2);
    });
    auto task2 = utils::Async("receive2", [this] {
        auto consumer = MakeConsumer(
            "kafka-consumer-second",
            /*topics=*/{kLargeTopic1, kLargeTopic2}
        );
        return ReceiveMessages(consumer, kMessagesCount / 2);
    });

    auto received1 = task1.Get();
    auto received2 = task2.Get();

    std::vector<kafka::utest::Message> received_messages;
    received_messages.reserve(kMessagesCount);
    std::move(received1.begin(), received1.end(), std::back_inserter(received_messages));
    std::move(received2.begin(), received2.end(), std::back_inserter(received_messages));

    EXPECT_THAT(received_messages, ::testing::UnorderedElementsAreArray(test_messages));
}

UTEST_F(ConsumerTest, OneConsumerPartitionDistribution) {
    const std::vector<kafka::utest::Message> test_messages{
        kafka::utest::Message{.topic = kLargeTopic1, .key = "key", .payload = "msg-1"},
        kafka::utest::Message{.topic = kLargeTopic1, .key = "key", .payload = "msg-2"},
        kafka::utest::Message{.topic = kLargeTopic1, .key = "key", .payload = "msg-3"},
    };
    SendMessages(test_messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{kLargeTopic1});

    const auto received_messages = ReceiveMessages(consumer, test_messages.size());

    std::unordered_set<std::uint32_t> partitions;
    for (const auto& message : received_messages) {
        ASSERT_TRUE(message.partition.has_value());
        partitions.emplace(*message.partition);
    }

    EXPECT_EQ(partitions.size(), 1ull);
}

UTEST_F(ConsumerTest, OneConsumerRereadAfterCommit) {
    const auto topic = GenerateTopic();
    const std::vector<kafka::utest::Message> test_messages{
        kafka::utest::Message{.topic = topic, .key = "key-1", .payload = "msg-1"},
        kafka::utest::Message{.topic = topic, .key = "key-2", .payload = "msg-2"},
        kafka::utest::Message{.topic = topic, .key = "key-3", .payload = "msg-3"},
    };
    SendMessages(test_messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic});
    const auto received_first = ReceiveMessages(
        consumer,
        test_messages.size(),
        /*commit_after_receive=*/false
    );

    const auto received_second = ReceiveMessages(
        consumer,
        test_messages.size(),
        /*commit_after_receive*/ true
    );

    EXPECT_EQ(received_first, received_second);
}

UTEST_F(ConsumerTest, LargeBatch) {
    constexpr std::size_t kMessagesCount{20};

    const auto topic = GenerateTopic();
    const auto messages = utils::GenerateFixedArray(kMessagesCount, [&topic](std::size_t i) {
        return kafka::utest::Message{
            .topic = topic,
            .key = fmt::format("key-{}", i),
            .payload = fmt::format("msg-{}", i)
        };
    });
    SendMessages(messages);

    auto consumer = MakeConsumer(
        "kafka-consumer",
        {topic},
        kafka::impl::ConsumerConfiguration{},
        kafka::impl::ConsumerExecutionParams{
            .max_batch_size = 100,
            .poll_timeout = utest::kMaxTestWaitTime / 2,
        }
    );
    auto consumer_scope = consumer.MakeConsumerScope();

    std::atomic<std::uint32_t> consumed{0};
    std::atomic<std::uint32_t> callback_calls{0};
    engine::SingleUseEvent consumed_event;
    consumer_scope.Start([&](kafka::MessageBatchView batch) {
        callback_calls.fetch_add(1);
        consumed.fetch_add(batch.size());

        if (consumed.load() == kMessagesCount) {
            consumed_event.Send();
        }
    });

    UEXPECT_NO_THROW(consumed_event.Wait());
    EXPECT_LT(callback_calls.load(), kMessagesCount) << callback_calls.load();
}

UTEST_F_MT(ConsumerTest, OneConsumerPartitionOffsets, 2) {
    constexpr std::size_t kMessagesCount{kNumPartitionsBlockingTopic + 1};
    constexpr std::uint32_t kFirstPartition{0};
    const auto messages = utils::GenerateFixedArray(kMessagesCount, [](std::size_t i) {
        return kafka::utest::Message{
            .topic = kBlockingTopic,
            .key = fmt::format("key-{}", i),
            .payload = fmt::format("msg-{}", i),
            .partition = i % kNumPartitionsBlockingTopic,
        };
    });
    SendMessages(messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{kBlockingTopic});
    {
        // 1. Check blocking function before consumer
        auto consumer_scope = consumer.MakeConsumerScope();
        const auto partitions = consumer_scope.GetPartitionIds(kBlockingTopic);
        EXPECT_EQ(partitions.size(), kNumPartitionsBlockingTopic);
    }

    ReceiveMessages(
        consumer,
        messages.size(),
        /*commit_after_receive=*/true,
        [&consumer](kafka::MessageBatchView batch) {
            // 2. Check blocking function when consumer processing
            const auto partitions = consumer.GetPartitionIds(kBlockingTopic);
            EXPECT_EQ(partitions.size(), kNumPartitionsBlockingTopic);

            for (const auto& msg : batch) {
                kafka::OffsetRange offset_range{};
                UEXPECT_NO_THROW(
                    offset_range = consumer.GetOffsetRange(msg.GetTopic(), msg.GetPartition(), utest::kMaxTestWaitTime)
                );
                EXPECT_EQ(offset_range.low, 0u);

                // Note that offset is not yet committed, just fetched
                if (msg.GetPartition() != kFirstPartition) {
                    EXPECT_EQ(offset_range.high, 1u);
                } else {
                    EXPECT_LE(offset_range.high, 2u);
                }
            }
        }
    );

    // 3. Check blocking function after consumer stopped
    const auto partitions = consumer.GetPartitionIds(kBlockingTopic);
    EXPECT_EQ(partitions.size(), kNumPartitionsBlockingTopic);

    for (const auto& partition_id : partitions) {
        kafka::OffsetRange offset_range{};
        UEXPECT_NO_THROW(offset_range = consumer.GetOffsetRange(kBlockingTopic, partition_id, utest::kMaxTestWaitTime));
        EXPECT_EQ(offset_range.low, 0u);
        if (partition_id != kFirstPartition) {
            EXPECT_EQ(offset_range.high, 1u);
        } else {
            EXPECT_LE(offset_range.high, 2u);
        }
    }
}

UTEST_F(ConsumerTest, HeadersProcessing) {
    static constexpr std::array
        kExpectedHeaders{kafka::HeaderView{"header-1", "value-1"}, kafka::HeaderView{"header-2", "value-2"}};
    const std::vector<kafka::OwningHeader> headers{kExpectedHeaders.begin(), kExpectedHeaders.end()};

    const auto topic = GenerateTopic();

    const std::array messages{kafka::utest::Message{
        .topic = topic,
        .key = "key",
        .payload = "value",
        .partition = kafka::kUnassignedPartition,
        .headers = headers
    }};
    SendMessages(messages);

    auto consumer = MakeConsumer("kafka-consumer", {topic});
    ReceiveMessages(
        consumer,
        /*expected_messages_count=*/messages.size(),
        /*commit_after_receive=*/true,
        [](kafka::MessageBatchView messages_batch) {
            ASSERT_EQ(messages_batch.size(), 1);

            /// [Message iterate headers]
            for (auto header : messages_batch[0].GetHeaders()) {
                EXPECT_TRUE(std::ranges::find(kExpectedHeaders, header) != kExpectedHeaders.end());
            }
            /// [Message iterate headers]
        }
    );
}

UTEST_F(ConsumerTest, DISABLED_IN_MAC_OS_TEST_NAME(SeekToBeginning)) {
    const auto topic = GenerateTopic();

    const std::array messages{
        kafka::utest::Message{.topic = topic, .key = "key-1", .payload = "value-1", .partition = 0},
        kafka::utest::Message{.topic = topic, .key = "key-2", .payload = "value-2", .partition = 0},
        kafka::utest::Message{.topic = topic, .key = "key-3", .payload = "value-3", .partition = 0},
        kafka::utest::Message{.topic = topic, .key = "key-4", .payload = "value-4", .partition = 0},
    };
    SendMessages(messages);

    // Emulate that consumer processed all messages from topic
    {
        auto consumer = MakeConsumer("kafka-consumer", {topic});
        ReceiveMessages(
            consumer,
            /*expected_messages_count=*/messages.size(),
            /*commit_after_receive=*/true
        );
    }

    // Check that seek to beginning works
    // All messages must be re-read after its call
    {
        auto consumer = MakeConsumer("kafka-consumer", {topic});

        engine::SingleUseEvent event;
        auto consumer_scope = consumer.MakeConsumerScope();
        auto rebalance_callback =
            [&consumer_scope, &event](kafka::TopicPartitionBatchView partitions, kafka::RebalanceEventType event_type) {
                if (event_type == kafka::RebalanceEventType::kAssigned) {
                    for (const auto& topic_partitions : partitions) {
                        UEXPECT_NO_THROW(consumer_scope.SeekToBeginning(
                            topic_partitions.topic,
                            topic_partitions.partition_id,
                            utest::kMaxTestWaitTime
                        ));
                    }

                    event.Send();
                }
            };
        consumer_scope.SetRebalanceCallback(std::move(rebalance_callback));

        const auto received_messages = ReceiveMessages(
            consumer_scope,
            /*expected_messages_count=*/messages.size(),
            /*commit_after_receive=*/true
        );

        // checks that callback was called after assign of partitions.
        event.Wait();
    }
}

UTEST_F(ConsumerTest, SeekToOffset) {
    constexpr std::size_t kMessagesCount{20};

    const auto topic = GenerateTopic();
    const auto messages = utils::GenerateFixedArray(kMessagesCount, [&topic](std::size_t i) {
        return kafka::utest::Message{
            .topic = topic,
            .key = fmt::format("key-{}", i),
            .payload = fmt::format("msg-{}", i)
        };
    });
    SendMessages(messages);

    auto consumer = MakeConsumer("kafka-consumer", {topic});

    engine::SingleUseEvent event;
    auto consumer_scope = consumer.MakeConsumerScope();

    // On partitions assign shift offset by `kMessagesToSkip`
    // After that, one excepts that consumer receives messages with offset greater of equal than `kMessagesToSkip`
    constexpr std::uint64_t kMessagesToSkip{7};
    auto rebalance_callback =
        [&consumer_scope, &event](kafka::TopicPartitionBatchView partitions, kafka::RebalanceEventType event_type) {
            if (event_type == kafka::RebalanceEventType::kAssigned) {
                for (const auto& topic_partitions : partitions) {
                    const auto offset_range = consumer_scope.GetOffsetRange(
                        topic_partitions.topic,
                        topic_partitions.partition_id,
                        utest::kMaxTestWaitTime
                    );
                    const auto offset_to_seek = offset_range.low + kMessagesToSkip;

                    UEXPECT_NO_THROW(consumer_scope.Seek(
                        topic_partitions.topic,
                        topic_partitions.partition_id,
                        offset_to_seek,
                        utest::kMaxTestWaitTime
                    ));
                }

                event.Send();
            }
        };
    consumer_scope.SetRebalanceCallback(std::move(rebalance_callback));

    const auto received_messages = ReceiveMessages(
        consumer,
        /*expected_messages_count=*/kMessagesCount - kMessagesToSkip,
        /*commit_after_receive=*/true
    );

    // checks that callback was called after assign of partitions.
    event.Wait();
    EXPECT_EQ(received_messages[0].key, messages[kMessagesToSkip].key);
}

UTEST_F(ConsumerTest, SeekToEnd) {
    const auto topic = GenerateTopic();

    const std::array before_messages{
        kafka::utest::Message{.topic = topic, .key = "key-1", .payload = "value-1", .partition = 0},
        kafka::utest::Message{.topic = topic, .key = "key-2", .payload = "value-2", .partition = 0},
    };
    const std::array after_seemessages{
        kafka::utest::Message{.topic = topic, .key = "key-3", .payload = "value-3", .partition = 0},
        kafka::utest::Message{.topic = topic, .key = "key-4", .payload = "value-4", .partition = 0},
        kafka::utest::Message{.topic = topic, .key = "key-5", .payload = "value-5", .partition = 0},
    };
    SendMessages(before_messages);

    // Send first batch of messages to topic
    // Asynchronously wait until rebalance callback called
    // When rebalance called, send one more batch of messages to topic
    // Expect that only that messages are read by consumer
    engine::SingleUseEvent seek_completed;
    auto task_send_after_seek =
        utils::Async("send_messages_after_seek", [this, &seek_completed, &after_seemessages]() mutable {
            // wait until seek occurs.
            seek_completed.Wait();

            // We need also to sleep for >= poll_timeout after seek, to send message only after seek occurs.
            engine::SleepFor(3 * kafka::impl::ConsumerExecutionParams{}.poll_timeout);
            SendMessages(after_seemessages);
        });

    auto consumer = MakeConsumer("kafka-consumer", {topic});
    auto consumer_scope = consumer.MakeConsumerScope();
    auto rebalance_callback =
        [&consumer_scope,
         &seek_completed](kafka::TopicPartitionBatchView partitions, kafka::RebalanceEventType event_type) {
            if (event_type == kafka::RebalanceEventType::kAssigned) {
                for (const auto& topic_partitions : partitions) {
                    UEXPECT_NO_THROW(consumer_scope.SeekToEnd(
                        topic_partitions.topic,
                        topic_partitions.partition_id,
                        utest::kMaxTestWaitTime
                    ));
                }

                // signals that seek have already occurred.
                seek_completed.Send();
            }
        };
    consumer_scope.SetRebalanceCallback(std::move(rebalance_callback));

    auto messages = ReceiveMessages(
        consumer_scope,
        /*expected_messages_count=*/after_seemessages.size(),
        /*commit_after_receive=*/true
    );

    ASSERT_EQ(messages.size(), after_seemessages.size());
    EXPECT_EQ(messages[0].key, after_seemessages[0].key);
}

UTEST_F(ConsumerTest, ConsumerRecreatesAfterFatalLibrdkafkaError) {
    static constexpr std::string_view kStaticGroupInstanceId{"utest-static-fence-instance"};
    static constexpr std::int32_t kHeartbeatApiKey = 12;
    static constexpr std::int32_t kFencedInstanceIdError = 82;

    const auto topic = GenerateTopic();

    kafka::impl::ConsumerExecutionParams params{};
    params.restart_after_failure_delay = std::chrono::milliseconds{100};

    kafka::impl::ConsumerConfiguration configuration{};
    configuration.rd_kafka_options["group.instance.id"] = kStaticGroupInstanceId;
    configuration.rd_kafka_options["session.timeout.ms"] = "6000";
    configuration.rd_kafka_options["heartbeat.interval.ms"] = "1000";

    const kafka::utest::Message first_message{
        .topic = topic,
        .key = "key-before-fatal",
        .payload = "payload-before-fatal",
        .partition = 0,
    };
    const kafka::utest::Message second_message{
        .topic = topic,
        .key = "key-after-fatal",
        .payload = "payload-after-fatal",
        .partition = 0,
    };

    SendMessages({first_message});

    auto consumer = MakeConsumer("kafka-consumer", {topic}, configuration, params);
    auto consumer_scope = consumer.MakeConsumerScope();

    std::vector<kafka::utest::Message> received_messages;
    engine::SingleUseEvent first_message_received;
    engine::SingleUseEvent second_message_received;

    consumer_scope.Start([&](kafka::MessageBatchView messages) {
        for (const auto& message : messages) {
            received_messages.push_back(kafka::utest::Message{
                message.GetTopic(),
                std::string{message.GetKey()},
                std::string{message.GetPayload()},
                message.GetPartition(),
            });
        }
        if (messages.empty()) {
            return;
        }
        consumer_scope.AsyncCommit();

        if (received_messages.size() == 1) {
            first_message_received.Send();
        }
        if (!received_messages.empty() && received_messages.back().key == second_message.key) {
            second_message_received.Send();
        }
    });

    first_message_received.Wait();

    // Real FENCEING does not occur on the kafka mock cluster during duplication group.instance.id
    // that's why we're emulating this behavior
    PushMockRequestError(kHeartbeatApiKey, kFencedInstanceIdError, /*count=*/1);
    // Heartbeat (1s) → fatal → close → restart delay (100ms) → new ConsumerImpl → JoinGroup → poll
    engine::SleepFor(std::chrono::milliseconds{2000});

    SendMessages({second_message});

    ASSERT_EQ(
        second_message_received.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
        engine::FutureStatus::kReady
    );

    consumer_scope.Stop();

    ASSERT_GE(received_messages.size(), 2U);
    EXPECT_EQ(received_messages.front().key, first_message.key);
    EXPECT_EQ(received_messages.back().key, second_message.key);
}

UTEST_F(ConsumerTest, HeadersSaveAfterMessageDestroy) {
    static constexpr std::array kExpectedHeaders{kafka::HeaderView{"header-1", "value-1"}};
    const std::vector<kafka::OwningHeader> headers{kExpectedHeaders.begin(), kExpectedHeaders.end()};

    const auto topic = GenerateTopic();

    const std::array messages{kafka::utest::Message{
        .topic = topic,
        .key = "key",
        .payload = "value",
        .partition = kafka::kUnassignedPartition,
        .headers = headers
    }};
    SendMessages(messages);

    std::vector<kafka::OwningHeader> saved_headers;
    auto consumer = MakeConsumer("kafka-consumer", {topic});
    ReceiveMessages(
        consumer,
        /*expected_messages_count=*/messages.size(),
        /*commit_after_receive=*/true,
        [&saved_headers](kafka::MessageBatchView batch) {
            ASSERT_EQ(batch.size(), 1);

            auto header = batch[0].GetHeader("header-1");
            ASSERT_TRUE(header.has_value());
            EXPECT_EQ(header, "value-1");
            EXPECT_FALSE(batch[0].GetHeader("header-2").has_value());

            /// [Message copy headers]
            auto reader = batch[0].GetHeaders();
            auto headers = std::vector<kafka::OwningHeader>{reader.begin(), reader.end()};
            /// [Message copy headers]
            saved_headers.insert(saved_headers.end(), headers.begin(), headers.end());
        }
    );

    ASSERT_EQ(saved_headers.size(), 1);
    EXPECT_EQ(saved_headers[0].GetName(), "header-1");
    EXPECT_EQ(saved_headers[0].GetValue(), "value-1");
}

USERVER_NAMESPACE_END
