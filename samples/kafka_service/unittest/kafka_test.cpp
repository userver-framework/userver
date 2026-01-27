#include <array>

#include <gmock/gmock-matchers.h>

#include <userver/utest/using_namespace_userver.hpp>

#include <consume.hpp>
#include <produce.hpp>

/// [Kafka service sample - kafka utest include]
#include <userver/kafka/utest/kafka_fixture.hpp>

class KafkaServiceTest : public kafka::utest::KafkaCluster {};
/// [Kafka service sample - kafka utest include]

/// [Kafka service sample - producer unit test]
UTEST_F(KafkaServiceTest, Produce) {
    auto producer = MakeProducer("kafka-producer");
    EXPECT_EQ(
        kafka_sample::Produce(producer, kafka_sample::RequestMessage{"test-topic", "test-key", "test-message"}),
        kafka_sample::SendStatus::kSuccess
    );
}
/// [Kafka service sample - producer unit test]

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ <= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
#endif

/// [Kafka service sample - consumer unit test]
UTEST_F(KafkaServiceTest, Consume) {
    const std::string test_topic1{"test-topic-1"};
    const std::string test_topic2{"test-topic-2"};
    const std::array test_messages{
        kafka::utest::Message{
            test_topic1,
            "test-key-1",
            "test-msg-1",
            /*partition=*/0
        },
        kafka::utest::Message{
            test_topic2,
            "test-key-2",
            "test-msg-2",
            /*partition=*/0
        }
    };

    SendMessages(test_messages);

    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{test_topic1, test_topic2});
    auto user_callback = [](kafka::MessageBatchView messages) { kafka_sample::Consume(messages); };
    const auto received_messages =
        ReceiveMessages(consumer, test_messages.size(), /* commit_after_receive */ true, std::move(user_callback));
    ASSERT_EQ(received_messages.size(), test_messages.size());
    EXPECT_THAT(received_messages, ::testing::UnorderedElementsAreArray(test_messages));
}
/// [Kafka service sample - consumer unit test]

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ <= 9
#pragma GCC diagnostic pop
#endif
