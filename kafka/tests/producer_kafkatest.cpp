#include "utils_kafkatest.hpp"

#include <deque>
#include <vector>

#include <fmt/format.h>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ProducerTest : public KafkaCluster {};

}  // namespace

UTEST_F(ProducerTest, OneProducerOneSendSync) {
  auto producer = MakeProducer("kafka-producer");
  UEXPECT_NO_THROW(producer.Send(GenerateTopic(), "test-key", "test-msg"));
}

UTEST_F(ProducerTest, OneProducerOneSendAsync) {
  auto producer = MakeProducer("kafka-producer");
  auto task = producer.SendAsync(GenerateTopic(), "test-key", "test-msg");
  UEXPECT_NO_THROW(task.Get());
}

UTEST_F(ProducerTest, BrokenConfiguration) {
  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.delivery_timeout = std::chrono::milliseconds{1};
  producer_configuration.queue_buffering_max = std::chrono::milliseconds{7};

  UEXPECT_THROW(MakeProducer("kafka-producer", producer_configuration),
                std::runtime_error);
}

UTEST_F(ProducerTest, LargeMessages) {
  constexpr std::size_t kSendCount{10};

  kafka::impl::ProducerConfiguration producer_configuration{};

  auto producer = MakeProducer("kafka-producer");
  const std::string topic = GenerateTopic();

  const std::string big_message(producer_configuration.message_max_bytes / 2,
                                'm');
  for (std::size_t send{0}; send < kSendCount; ++send) {
    UEXPECT_NO_THROW(
        producer.Send(topic, fmt::format("test-key-{}", send), big_message));
  }
}

UTEST_F(ProducerTest, TooLargeMessage) {
  constexpr std::uint32_t kMessageMaxBytes{3000};

  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.message_max_bytes = kMessageMaxBytes;
  producer_configuration.rd_kafka_options["debug"] = "all";

  auto producer = MakeProducer("kafka-producer", producer_configuration);
  UEXPECT_NO_THROW(
      producer.Send(GenerateTopic(), "small-key", "small-message"));

  const std::string big_key(kMessageMaxBytes, 'k');
  const std::string big_message(kMessageMaxBytes, 'm');
  UEXPECT_THROW(producer.Send(GenerateTopic(), big_key, big_message),
                kafka::MessageTooLargeException);
}

UTEST_F(ProducerTest, UnknownPartition) {
  auto producer = MakeProducer("kafka-producer");
  UEXPECT_THROW(producer.Send(GenerateTopic(), "test-key", "test-msg",
                              /*partition=*/100500),
                kafka::UnknownPartitionException);
}

UTEST_F(ProducerTest, FullQueue) {
  constexpr std::uint32_t kMaxQueueMessages{7};

  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.delivery_timeout = std::chrono::seconds{6};
  producer_configuration.queue_buffering_max = std::chrono::seconds{3};
  producer_configuration.queue_buffering_max_messages = kMaxQueueMessages;

  auto producer = MakeProducer("kafka-producer", producer_configuration);
  const std::string topic = GenerateTopic();

  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(kMaxQueueMessages);
  for (std::uint32_t send{0}; send < kMaxQueueMessages; ++send) {
    results.emplace_back(producer.SendAsync(topic,
                                            fmt::format("test-key-{}", send),
                                            fmt::format("test-msg-{}", send)));
  }
  auto make_send_request =
      [&producer, &topic, key = fmt::format("test-key-{}", kMaxQueueMessages),
       message = fmt::format("test-msg-{}", kMaxQueueMessages)] {
        producer.Send(topic, key, message);
      };

  UEXPECT_THROW(make_send_request(), kafka::QueueFullException);

  /// [Producer retryable error]
  bool delivered{false};
  const auto deadline =
      engine::Deadline::FromDuration(producer_configuration.delivery_timeout);
  while (!delivered && !deadline.IsReached()) {
    try {
      make_send_request();
      delivered = true;
    } catch (const kafka::SendException& e) {
      if (e.IsRetryable()) {
        engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
        continue;
      }
      break;
    }
  }
  /// [Producer retryable error]

  EXPECT_TRUE(delivered);
  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
}

UTEST_F(ProducerTest, SendCancel) {
  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.delivery_timeout = std::chrono::seconds{6};
  producer_configuration.queue_buffering_max = std::chrono::seconds{3};

  auto producer = MakeProducer("kafka-producer", producer_configuration);

  auto send_task = producer.SendAsync(GenerateTopic(), "test-key", "test-msg");
  send_task.RequestCancel();
  UEXPECT_NO_THROW(send_task.Wait());
}

UTEST_F(ProducerTest, WaitingForMessageDelivery) {
  constexpr std::size_t kSendCount{100};

  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.delivery_timeout = std::chrono::milliseconds{1500};
  producer_configuration.queue_buffering_max = std::chrono::milliseconds{1000};

  auto producer = std::make_unique<kafka::Producer>(
      utils::LazyPrvalue([this] { return MakeProducer("kafka-producer"); }));

  auto send_tasks = utils::GenerateFixedArray(
      kSendCount, [&producer, topic = GenerateTopic()](std::size_t i) {
        return producer->SendAsync(topic, fmt::format("test-key-{}", i),
                                   fmt::format("test-msg-{}", i));
      });
  /// queue_buffering_max is set to 1 seconds, therefore SendAsyncs waits for
  /// at least 1 second, so `producer.reset()` are invoked faster than
  /// SendAsyncs deliver the message.
  auto destroy_task = engine::AsyncNoSpan(
      [producer = std::move(producer)]() mutable { producer.reset(); });

  UEXPECT_NO_THROW(destroy_task.Get());
  UEXPECT_NO_THROW(engine::WaitAllChecked(send_tasks));
}

UTEST_F(ProducerTest, OneProducerManySendSync) {
  constexpr std::size_t kSendCount{100};

  auto producer = MakeProducer("kafka-producer");
  const auto topic = GenerateTopic();

  for (std::size_t send{0}; send < kSendCount; ++send) {
    UEXPECT_NO_THROW(producer.Send(topic, fmt::format("test-key-{}", send),
                                   fmt::format("test-msg-{}", send)))
        << send;
  }
}

UTEST_F(ProducerTest, OneProducerManySendAsync) {
  constexpr std::size_t kSendCount{100};

  auto producer = MakeProducer("kafka-producer");
  const auto topic = GenerateTopic();

  /// [Producer batch send async]
  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(kSendCount);
  for (std::size_t send{0}; send < kSendCount; ++send) {
    results.emplace_back(producer.SendAsync(topic,
                                            fmt::format("test-key-{}", send),
                                            fmt::format("test-msg-{}", send)));
  }

  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
  /// [Producer batch send async]
}

UTEST_F(ProducerTest, ManyProducersManySendSync) {
  constexpr std::size_t kProducerCount{4};
  constexpr std::size_t kSendCount{100};
  constexpr std::size_t kTopicCount{kSendCount / 10};

  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.queue_buffering_max = std::chrono::seconds{0};
  const std::deque<kafka::Producer> producers = MakeProducers(
      kProducerCount,
      [](std::size_t i) { return fmt::format("kafka-producer-{}", i); },
      producer_configuration);
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  for (std::size_t send{0}; send < kSendCount; ++send) {
    const auto& topic = topics.at(send % kTopicCount);
    auto& producer = producers.at(send % kProducerCount);
    UEXPECT_NO_THROW(producer.Send(topic, fmt::format("test-key-{}", send),
                                   fmt::format("test-msg-{}", send)))
        << send;
  }
}

UTEST_F(ProducerTest, ManyProducersManySendAsyncSingleThread) {
  constexpr std::size_t kProducerCount{10};
  constexpr std::size_t kSendCount{1000};

  const std::deque<kafka::Producer> producers = MakeProducers(
      kProducerCount,
      [](std::size_t i) { return fmt::format("kafka-producer-{}", i); });
  const std::string topic = GenerateTopic();

  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(kSendCount);
  for (std::size_t send{0}; send < kSendCount; ++send) {
    auto& producer = producers.at(send % kProducerCount);
    results.emplace_back(producer.SendAsync(topic,
                                            fmt::format("test-key-{}", send),
                                            fmt::format("test-msg-{}", send)));
  }

  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
}

UTEST_F_MT(ProducerTest, ManyProducersManySendAsync, 4 + 4) {
  constexpr std::size_t kProducerCount{4};
  constexpr std::size_t kSendCount{300};
  constexpr std::size_t kTopicCount{kSendCount / 10};

  const std::deque<kafka::Producer> producers = MakeProducers(
      kProducerCount,
      [](std::size_t i) { return fmt::format("kafka-producer-{}", i); });
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(kSendCount);
  for (std::size_t send{0}; send < kSendCount; ++send) {
    const auto& topic = topics.at(send % kTopicCount);
    auto& producer = producers.at(send % kProducerCount);
    results.emplace_back(producer.SendAsync(topic,
                                            fmt::format("test-key-{}", send),
                                            fmt::format("test-msg-{}", send)));
  }

  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
}

UTEST_F_MT(ProducerTest, OneProducerManySendSyncMt, 1 + 4) {
  auto producer = MakeProducer("kafka-producer");

  constexpr std::size_t kSendCount{100};
  constexpr std::size_t kTopicCount{4};
  constexpr std::size_t kSendPerTask{kSendCount / kTopicCount};
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(GetThreadCount());
  for (std::size_t group{0}; group + 1 < GetThreadCount(); ++group) {
    results.emplace_back(utils::Async(
        fmt::format("producer_test_send_sync_{}", group), [&producer, &topics] {
          for (std::size_t send{0}; send < kSendPerTask; ++send) {
            producer.Send(topics.at(send % topics.size()),
                          fmt::format("test-key-{}", send),
                          fmt::format("test-msg-{}", send));
          }
        }));
  }

  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
}

UTEST_F_MT(ProducerTest, OneProducerManySendAsyncMt, 1 + 4) {
  auto producer = MakeProducer("kafka-producer");

  constexpr std::size_t kSendCount{1000};
  constexpr std::size_t kTopicCount{4};
  constexpr std::size_t kSendPerTask{kSendCount / kTopicCount};
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  std::vector<engine::TaskWithResult<void>> parallel_tasks;
  parallel_tasks.reserve(GetThreadCount());
  concurrent::Variable<std::vector<engine::TaskWithResult<void>>> results;
  for (std::size_t group{0}; group + 1 < GetThreadCount(); ++group) {
    parallel_tasks.emplace_back(utils::Async(
        fmt::format("producer_test_send_async_{}", group),
        [&producer, &results, &topics] {
          for (std::size_t send{0}; send < kSendPerTask; ++send) {
            auto task = producer.SendAsync(topics.at(send % topics.size()),
                                           fmt::format("test-key-{}", send),
                                           fmt::format("test-msg-{}", send));
            auto results_lock = results.Lock();
            results_lock->push_back(std::move(task));
          }
        }));
  }
  UEXPECT_NO_THROW(engine::WaitAllChecked(parallel_tasks));

  auto results_lock = results.Lock();
  UEXPECT_NO_THROW(engine::WaitAllChecked(*results_lock));
}

USERVER_NAMESPACE_END
