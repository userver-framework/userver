#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <vector>

#include <userver/kafka/impl/broker_secrets.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/consumer.hpp>
#include <userver/kafka/producer.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::utest {

/// @ingroup userver_utest
///
/// @brief Message owning data wrapper for unit tests.
struct Message {
  std::string topic;
  std::string key;
  std::string payload;
  std::optional<std::uint32_t> partition;
};

bool operator==(const Message& lhs, const Message& rhs);

/// @ingroup userver_utest
///
/// @brief Helper for Kafka unit testing.
///
/// KafkaCluster is useful for
/// - inline producer, consumer and their configuration creation;
/// - generation of unique topic names;
/// - sending and receiving batched of messages in sync form;
///
/// KafkaCluster expects that topic names used in test are already exist in
/// Kafka broker or auto-created on produce to it.
///
/// It is recommended to create all topics before tests started, because
/// topic creation leads to disk IO operation and maybe too slow in tests'
/// runtime.
///
/// If one run tests with Kafka testsuite plugin, use
/// `TESTSUITE_KAFKA_CUSTOM_TOPICS` environment variable to create topics before
/// tests started.
///
/// @note KafkaCluster determines the broker's connection string
/// based on `TESTSUITE_KAFKA_SERVER_HOST` (by default `localhost`) and
/// `TESTSUITE_KAFKA_SERVER_PORT` (by default `9099`) environment variables.
///
/// @see https://yandex.github.io/yandex-taxi-testsuite/kafka
class KafkaCluster : public ::testing::Test {
 public:
  /// Kafka broker has some cold start issues on its. To stabilize tests
  /// KafkaCluster patches producer's default delivery timeout.
  /// To use custom delivery timeout in test, pass `configuration` argument to
  /// MakeProducer.
  static constexpr const std::chrono::milliseconds kDefaultTestProducerTimeout{
      USERVER_NAMESPACE::utest::kMaxTestWaitTime / 2};

  KafkaCluster();

  ~KafkaCluster() override = default;

  std::string GenerateTopic();

  std::vector<std::string> GenerateTopics(std::size_t count);

  impl::Configuration MakeProducerConfiguration(
      const std::string& name, impl::ProducerConfiguration configuration = {},
      impl::Secret secrets = {});

  impl::Configuration MakeConsumerConfiguration(
      const std::string& name, impl::ConsumerConfiguration configuration = {},
      impl::Secret secrets = {});

  Producer MakeProducer(const std::string& name,
                        impl::ProducerConfiguration configuration = {});

  std::deque<Producer> MakeProducers(
      std::size_t count, std::function<std::string(std::size_t)> nameGenerator,
      impl::ProducerConfiguration configuration = {});

  /// @brief Creates temporary producer and send messages span.
  void SendMessages(utils::span<const Message> messages);

  impl::Consumer MakeConsumer(const std::string& name,
                              const std::vector<std::string>& topics,
                              impl::ConsumerConfiguration configuration = {},
                              impl::ConsumerExecutionParams params = {});

  /// @brief Starts consumer, waits until `expected_message_count` messages are
  /// consumed, calls `user_callback` if set, stops consumer.
  std::vector<Message> ReceiveMessages(
      impl::Consumer& consumer, std::size_t expected_messages_count,
      bool commit_after_receive = true,
      std::optional<std::function<void(MessageBatchView)>> user_callback = {});

 private:
  impl::Secret AddBootstrapServers(impl::Secret secrets) const;

 private:
  std::atomic<std::size_t> topics_count_{0};

  const std::string bootstrap_servers_;
};

}  // namespace kafka::utest

USERVER_NAMESPACE_END
