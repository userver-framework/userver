#pragma once

#include <deque>
#include <iostream>
#include <vector>

#include <userver/kafka/producer.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/span.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/configuration.hpp>
#include <kafka/impl/consumer.hpp>

USERVER_NAMESPACE_BEGIN

struct Message {
  std::string topic;
  std::string key;
  std::string payload;
  std::optional<std::uint32_t> partition;

  bool operator==(const Message& other) const = default;
};

std::ostream& operator<<(std::ostream&, const Message&);

class KafkaCluster : public ::testing::Test {
 public:
  KafkaCluster();

  ~KafkaCluster() override = default;

  std::string GenerateTopic();

  std::vector<std::string> GenerateTopics(std::size_t count);

  kafka::impl::Configuration MakeProducerConfiguration(
      const std::string& name,
      kafka::impl::ProducerConfiguration configuration = {},
      kafka::impl::Secret secrets = {});

  kafka::impl::Configuration MakeConsumerConfiguration(
      const std::string& name,
      kafka::impl::ConsumerConfiguration configuration = {},
      kafka::impl::Secret secrets = {});

  kafka::Producer MakeProducer(
      const std::string& name,
      kafka::impl::ProducerConfiguration configuration = {});

  std::deque<kafka::Producer> MakeProducers(
      std::size_t count, std::function<std::string(std::size_t)> nameGenerator,
      kafka::impl::ProducerConfiguration configuration = {});

  void SendMessages(utils::span<const Message> messages);

  kafka::impl::Consumer MakeConsumer(
      const std::string& name,
      const std::vector<std::string>& topics = {"test-topic"},
      kafka::impl::ConsumerConfiguration configuration = {},
      kafka::impl::ConsumerExecutionParams params = {});

 private:
  kafka::impl::Secret AddBootstrapServers(kafka::impl::Secret secrets) const;

 private:
  std::atomic<std::size_t> topics_count_{0};

  const std::string bootstrap_servers_;
};

USERVER_NAMESPACE_END
