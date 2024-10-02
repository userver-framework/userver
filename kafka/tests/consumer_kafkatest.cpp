#include <userver/kafka/utest/kafka_fixture.hpp>

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>

#include <gmock/gmock.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ConsumerTest : public kafka::utest::KafkaCluster {};

const std::string kLargeTopic1{"lt-1"};
const std::string kLargeTopic2{"lt-2"};

constexpr std::size_t kNumPartitionsLargeTopic{4};

std::vector<kafka::utest::Message> ReceiveMessages(
    kafka::impl::Consumer& consumer, std::size_t expected_messages_count,
    bool commit = true) {
  std::vector<kafka::utest::Message> received_messages;

  engine::SingleUseEvent event;
  auto consumer_scope = consumer.MakeConsumerScope();
  consumer_scope.Start([&received_messages, expected_messages_count, &event,
                        &consumer_scope,
                        commit](kafka::MessageBatchView messages) {
    for (const auto& message : messages) {
      received_messages.push_back(kafka::utest::Message{
          message.GetTopic(), std::string{message.GetKey()},
          std::string{message.GetPayload()}, message.GetPartition()});
    }
    if (commit) {
      consumer_scope.AsyncCommit();
    }

    if (received_messages.size() == expected_messages_count) {
      event.Send();
    }
  });

  event.Wait();

  return received_messages;
}

}  // namespace

UTEST_F(ConsumerTest, BrokenConfiguration) {
  kafka::impl::ConsumerConfiguration consumer_configuration{};
  consumer_configuration.max_callback_duration = std::chrono::milliseconds{10};
  consumer_configuration.rd_kafka_options["session.timeout.ms"] = "10000";

  UEXPECT_THROW(MakeConsumer("kafka-consumer", {}, consumer_configuration),
                std::runtime_error);
}

UTEST_F(ConsumerTest, OneConsumerSmallTopics) {
  const auto topic1 = GenerateTopic();
  const auto topic2 = GenerateTopic();

  const std::vector<kafka::utest::Message> kTestMessages{
      kafka::utest::Message{topic1, "key-1", "msg-1", /*partition=*/0},
      kafka::utest::Message{topic1, "key-2", "msg-2", /*partition=*/0},
      kafka::utest::Message{topic2, "key-3", "msg-3", /*partition=*/0},
      kafka::utest::Message{topic2, "key-4", "msg-4", /*partition=*/0}};
  SendMessages(kTestMessages);

  auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic1, topic2});

  const auto received_messages =
      ReceiveMessages(consumer, kTestMessages.size());

  EXPECT_THAT(received_messages,
              ::testing::UnorderedElementsAreArray(kTestMessages));
}

UTEST_F(ConsumerTest, OneConsumerLargeTopics) {
  constexpr std::size_t kMessagesCount{2 * 2 * kNumPartitionsLargeTopic};

  std::vector<kafka::utest::Message> kTestMessages{kMessagesCount};
  std::generate_n(kTestMessages.begin(), kMessagesCount, [i = 0]() mutable {
    i += 1;
    return kafka::utest::Message{i % 2 == 0 ? kLargeTopic1 : kLargeTopic2,
                                 fmt::format("key-{}", i),
                                 fmt::format("msg-{}", i),
                                 /*partition=*/i % kNumPartitionsLargeTopic};
  });
  SendMessages(kTestMessages);

  auto consumer =
      MakeConsumer("kafka-consumer", /*topics=*/{kLargeTopic1, kLargeTopic2});

  const auto received_messages =
      ReceiveMessages(consumer, kTestMessages.size());

  EXPECT_THAT(received_messages,
              ::testing::UnorderedElementsAreArray(kTestMessages));
}

UTEST_F(ConsumerTest, ManyConsumersLargeTopics) {
  constexpr std::size_t kMessagesCount{2 * 2 * kNumPartitionsLargeTopic};

  std::vector<kafka::utest::Message> kTestMessages{kMessagesCount};
  std::generate_n(kTestMessages.begin(), kMessagesCount, [i = 0]() mutable {
    i += 1;
    return kafka::utest::Message{i % 2 == 0 ? kLargeTopic1 : kLargeTopic2,
                                 fmt::format("key-{}", i),
                                 fmt::format("msg-{}", i),
                                 /*partition=*/i % kNumPartitionsLargeTopic};
  });
  SendMessages(kTestMessages);

  auto task1 = utils::Async("receive1", [this] {
    auto consumer = MakeConsumer("kafka-consumer-first",
                                 /*topics=*/{kLargeTopic1, kLargeTopic2});
    return ReceiveMessages(consumer, kMessagesCount / 2);
  });
  auto task2 = utils::Async("receive2", [this] {
    auto consumer = MakeConsumer("kafka-consumer-second",
                                 /*topics=*/{kLargeTopic1, kLargeTopic2});
    return ReceiveMessages(consumer, kMessagesCount / 2);
  });

  auto received1 = task1.Get();
  auto received2 = task2.Get();

  std::vector<kafka::utest::Message> received_messages;
  received_messages.reserve(kMessagesCount);
  std::move(received1.begin(), received1.end(),
            std::back_inserter(received_messages));
  std::move(received2.begin(), received2.end(),
            std::back_inserter(received_messages));

  EXPECT_THAT(received_messages,
              ::testing::UnorderedElementsAreArray(kTestMessages));
}

UTEST_F(ConsumerTest, OneConsumerPartitionDistribution) {
  const std::vector<kafka::utest::Message> kTestMessages{
      kafka::utest::Message{kLargeTopic1, "key", "msg-1",
                            /*partition=*/std::nullopt},
      kafka::utest::Message{kLargeTopic1, "key", "msg-2",
                            /*partition=*/std::nullopt},
      kafka::utest::Message{kLargeTopic1, "key", "msg-3",
                            /*partition=*/std::nullopt}};
  SendMessages(kTestMessages);

  auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{kLargeTopic1});

  const auto received_messages =
      ReceiveMessages(consumer, kTestMessages.size());

  std::unordered_set<std::uint32_t> partitions;
  for (const auto& message : received_messages) {
    ASSERT_TRUE(message.partition.has_value());
    partitions.emplace(*message.partition);
  }

  EXPECT_EQ(partitions.size(), 1ull);
}

UTEST_F(ConsumerTest, OneConsumerRereadAfterCommit) {
  const auto topic = GenerateTopic();
  const std::vector<kafka::utest::Message> kTestMessages{
      kafka::utest::Message{topic, "key-1", "msg-1",
                            /*partition=*/std::nullopt},
      kafka::utest::Message{topic, "key-2", "msg-2",
                            /*partition=*/std::nullopt},
      kafka::utest::Message{topic, "key-3", "msg-3",
                            /*partition=*/std::nullopt}};
  SendMessages(kTestMessages);

  auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic});
  const auto received_first =
      ReceiveMessages(consumer, kTestMessages.size(), /*commit=*/false);
  const auto received_second =
      ReceiveMessages(consumer, kTestMessages.size(), /*commit=*/true);

  EXPECT_EQ(received_first, received_second);
}

UTEST_F(ConsumerTest, LargeBatch) {
  constexpr std::size_t kMessagesCount{20};

  const auto topic = GenerateTopic();
  const auto messages =
      utils::GenerateFixedArray(kMessagesCount, [&topic](std::size_t i) {
        return kafka::utest::Message{topic, fmt::format("key-{}", i),
                                     fmt::format("msg-{}", i), std::nullopt};
      });
  SendMessages(messages);

  auto consumer = MakeConsumer(
      "kafka-consumer", {topic}, kafka::impl::ConsumerConfiguration{},
      kafka::impl::ConsumerExecutionParams{
          /*max_batch_size=*/100,
          /*poll_timeout=*/utest::kMaxTestWaitTime / 2});
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

USERVER_NAMESPACE_END
