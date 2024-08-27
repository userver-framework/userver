#include "test_utils.hpp"

#include <deque>
#include <vector>

#include <fmt/format.h>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ProducerTest : public KafkaCluster {};

}  // namespace

UTEST_F_MT(ProducerTest, OneProducerOneSendSync, 2) {
  auto producer = MakeProducer("kafka-producer");
  UEXPECT_NO_THROW(producer.Send(GenerateTopic(), "test-key", "test-msg"));
}

UTEST_F_MT(ProducerTest, OneProducerOneSendAsync, 2) {
  auto producer = MakeProducer("kafka-producer");
  auto task = producer.SendAsync(GenerateTopic(), "test-key", "test-msg");
  UEXPECT_NO_THROW(task.Get());
}

UTEST_F_MT(ProducerTest, OneProducerManySendSync, 2) {
  auto producer = MakeProducer("kafka-producer");

  constexpr std::size_t kSendCount{100};

  const auto topic = GenerateTopic();
  for (std::size_t send{0}; send < kSendCount; ++send) {
    UEXPECT_NO_THROW(producer.Send(topic, fmt::format("test-key-{}", send),
                                   fmt::format("test-msg-{}", send)));
  }
}

UTEST_F_MT(ProducerTest, OneProducerManySendAsync, 2) {
  auto producer = MakeProducer("kafka-producer");

  constexpr std::size_t kSendCount{100};

  const auto topic = GenerateTopic();

  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(kSendCount);
  for (std::size_t send{0}; send < kSendCount; ++send) {
    results.emplace_back(producer.SendAsync(topic,
                                            fmt::format("test-key-{}", send),
                                            fmt::format("test-msg-{}", send)));
  }

  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
}

UTEST_F_MT(ProducerTest, ManyProducersManySendSync, 1 + 4) {
  constexpr std::size_t kProducerCount{4};
  const std::deque<kafka::Producer> producers = MakeProducers(
      kProducerCount,
      [](std::size_t i) { return fmt::format("kafka-producer-{}", i); });

  constexpr std::size_t kSendCount{300};
  constexpr std::size_t kTopicCount{kSendCount / 10};
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  for (std::size_t send{0}; send < kSendCount; ++send) {
    const auto& topic = topics.at(send % kTopicCount);
    auto& producer = producers.at(send % kProducerCount);
    UEXPECT_NO_THROW(producer.Send(topic, fmt::format("test-key-{}", send),
                                   fmt::format("test-msg-{}", send)));
  }
}

UTEST_F_MT(ProducerTest, ManyProducersManySendAsync, 4 + 4) {
  constexpr std::size_t kProducerCount{4};

  const std::deque<kafka::Producer> producers = MakeProducers(
      kProducerCount,
      [](std::size_t i) { return fmt::format("kafka-producer-{}", i); });

  constexpr std::size_t kSendCount{300};
  constexpr std::size_t kTopicCount{kSendCount / 10};
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  std::vector<engine::TaskWithResult<void>> results;
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
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  std::vector<engine::TaskWithResult<void>> results;
  results.reserve(kSendCount);
  for (std::size_t send{0}; send < kSendCount; ++send) {
    results.emplace_back(
        utils::Async("producer_test_send_send",
                     [send, &producer, &topic = topics.at(send % kTopicCount)] {
                       producer.Send(topic, fmt::format("test-key-{}", send),
                                     fmt::format("test-msg-{}", send));
                     }));
  }

  UEXPECT_NO_THROW(engine::WaitAllChecked(results));
}

UTEST_F_MT(ProducerTest, OneProducerManySendAsyncMt, 1 + 4) {
  auto producer = MakeProducer("kafka-producer");

  constexpr std::size_t kSendCount{100};
  constexpr std::size_t kTopicCount{4};
  constexpr std::size_t kSendPerTask{kSendCount / kTopicCount};
  const std::vector<std::string> topics = GenerateTopics(kTopicCount);

  std::vector<engine::TaskWithResult<void>> parallel_tasks;
  concurrent::Variable<std::vector<engine::TaskWithResult<void>>> results;
  for (std::size_t group{0}; group < kTopicCount; ++group) {
    parallel_tasks.emplace_back(utils::Async(
        "producer_test_send_async", [&producer, &results, &topics] {
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
