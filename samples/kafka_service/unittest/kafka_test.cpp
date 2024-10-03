#include <userver/kafka/utest/kafka_fixture.hpp>

#include <array>

#include <gmock/gmock-matchers.h>

#include <userver/utest/using_namespace_userver.hpp>

#include <consume.hpp>
#include <produce.hpp>

class KafkaServiceTest : public kafka::utest::KafkaCluster {};

UTEST_F(KafkaServiceTest, Produce) {
  auto producer = MakeProducer("kafka-producer");
  EXPECT_EQ(kafka_sample::Produce(
                producer, kafka_sample::RequestMessage{"test-topic", "test-key",
                                                       "test-message"}),
            kafka_sample::SendStatus::kSuccess);
}

UTEST_F(KafkaServiceTest, Consume) {
  const std::string kTestTopic1{"test-topic-1"};
  const std::string kTestTopic2{"test-topic-2"};
  const std::array kTestMessages{
      kafka::utest::Message{kTestTopic1, "test-key-1", "test-msg-1",
                            /*partition=*/0},
      kafka::utest::Message{kTestTopic2, "test-key-2", "test-msg-2",
                            /*partition=*/0}};

  SendMessages(kTestMessages);

  auto consumer =
      MakeConsumer("kafka-consumer", /*topics=*/{kTestTopic1, kTestTopic2});
  const auto received_messages =
      ReceiveMessages(consumer, kTestMessages.size());
  ASSERT_EQ(received_messages.size(), kTestMessages.size());
  EXPECT_THAT(received_messages,
              ::testing::UnorderedElementsAreArray(kTestMessages));
}
