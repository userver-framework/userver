#pragma once

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

struct Message {
  std::string topic;
  std::string key;
  std::string payload;
  std::optional<std::uint32_t> partition;
};

bool operator==(const Message& lhs, const Message& rhs);

class KafkaCluster : public ::testing::Test {
 public:
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

  void SendMessages(utils::span<const Message> messages);

  impl::Consumer MakeConsumer(
      const std::string& name,
      const std::vector<std::string>& topics = {"test-topic"},
      impl::ConsumerConfiguration configuration = {},
      impl::ConsumerExecutionParams params = {});

 private:
  impl::Secret AddBootstrapServers(impl::Secret secrets) const;

 private:
  std::atomic<std::size_t> topics_count_{0};

  const std::string bootstrap_servers_;
};

}  // namespace kafka::utest

USERVER_NAMESPACE_END
