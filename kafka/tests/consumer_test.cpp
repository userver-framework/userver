#include "test_utils.hpp"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include <gmock/gmock.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ConsumerTest : public KafkaCluster {};

const std::string kLargeTopic1{"large-topic-1"};
const std::string kLargeTopic2{"large-topic-2"};

constexpr std::size_t kNumPartitionsLargeTopic{4};

std::vector<Message> ReceiveMessages(kafka::impl::Consumer& consumer,
                                     std::size_t expected_messages_count,
                                     bool commit = true) {
  std::vector<Message> received_messages;

  engine::SingleUseEvent event;
  auto consumer_scope = consumer.MakeConsumerScope();
  consumer_scope.Start([&received_messages, expected_messages_count, &event,
                        &consumer_scope,
                        commit](kafka::MessageBatchView messages) {
    for (const auto& message : messages) {
      received_messages.push_back(
          Message{message.GetTopic(), std::string{message.GetKey()},
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

UTEST_F_MT(ConsumerTest, OneConsumerSmallTopics, 2) {
  const auto topic1 = GenerateTopic();
  const auto topic2 = GenerateTopic();

  const std::vector<Message> kTestMessages{
      Message{topic1, "key-1", "msg-1", /*partition=*/0},
      Message{topic1, "key-2", "msg-2", /*partition=*/0},
      Message{topic2, "key-3", "msg-3", /*partition=*/0},
      Message{topic2, "key-4", "msg-4", /*partition=*/0}};
  SendMessages(kTestMessages);

  auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic1, topic2});

  const auto received_messages =
      ReceiveMessages(consumer, kTestMessages.size());

  EXPECT_THAT(received_messages,
              ::testing::UnorderedElementsAreArray(kTestMessages));
}

UTEST_F_MT(ConsumerTest, OneConsumerLargeTopics, 2) {
  constexpr std::size_t kMessagesCount{2 * 2 * kNumPartitionsLargeTopic};

  std::vector<Message> kTestMessages{kMessagesCount};
  std::generate_n(kTestMessages.begin(), kMessagesCount, [i = 0]() mutable {
    i += 1;
    return Message{i % 2 == 0 ? kLargeTopic1 : kLargeTopic2,
                   fmt::format("key-{}", i), fmt::format("msg-{}", i),
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

UTEST_F_MT(ConsumerTest, ManyConsumersLargeTopics, 3) {
  constexpr std::size_t kMessagesCount{2 * 2 * kNumPartitionsLargeTopic};

  std::vector<Message> kTestMessages{kMessagesCount};
  std::generate_n(kTestMessages.begin(), kMessagesCount, [i = 0]() mutable {
    i += 1;
    return Message{i % 2 == 0 ? kLargeTopic1 : kLargeTopic2,
                   fmt::format("key-{}", i), fmt::format("msg-{}", i),
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

  std::vector<Message> received_messages;
  received_messages.reserve(kMessagesCount);
  std::move(received1.begin(), received1.end(),
            std::back_inserter(received_messages));
  std::move(received2.begin(), received2.end(),
            std::back_inserter(received_messages));

  EXPECT_THAT(received_messages,
              ::testing::UnorderedElementsAreArray(kTestMessages));
}

UTEST_F_MT(ConsumerTest, OneConsumerPartitionDistribution, 2) {
  const std::vector<Message> kTestMessages{
      Message{kLargeTopic1, "key", "msg-1", /*partition=*/std::nullopt},
      Message{kLargeTopic1, "key", "msg-2", /*partition=*/std::nullopt},
      Message{kLargeTopic1, "key", "msg-3", /*partition=*/std::nullopt}};
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

UTEST_F_MT(ConsumerTest, OneConsumerRereadAfterCommit, 2) {
  const auto topic = GenerateTopic();
  const std::vector<Message> kTestMessages{
      Message{topic, "key-1", "msg-1", /*partition=*/std::nullopt},
      Message{topic, "key-2", "msg-2", /*partition=*/std::nullopt},
      Message{topic, "key-3", "msg-3", /*partition=*/std::nullopt}};
  SendMessages(kTestMessages);

  std::vector<Message> received_first;
  std::vector<Message> received_second;
  {
    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic});
    received_first =
        ReceiveMessages(consumer, kTestMessages.size(), /*commit=*/false);
  }
  {
    auto consumer = MakeConsumer("kafka-consumer", /*topics=*/{topic});
    received_second =
        ReceiveMessages(consumer, kTestMessages.size(), /*commit=*/true);
  }

  EXPECT_EQ(received_first, received_second);
}

USERVER_NAMESPACE_END
