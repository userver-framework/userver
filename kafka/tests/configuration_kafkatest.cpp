#include "utils_kafkatest.hpp"

#include <optional>

#include <gmock/gmock.h>
#include <librdkafka/rdkafka.h>

#include <userver/engine/subprocess/environment_variables.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using namespace std::chrono_literals;

class ConfigurationTest : public KafkaCluster {};

}  // namespace

UTEST_F(ConfigurationTest, Producer) {
  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(
      configuration.emplace(MakeProducerConfiguration("kafka-producer")));

  const kafka::impl::ProducerConfiguration default_producer{};
  EXPECT_EQ(configuration->GetOption("client.id"),
            default_producer.common.client_id);
  EXPECT_EQ(
      configuration->GetOption("topic.metadata.refresh.interval.ms"),
      std::to_string(
          default_producer.common.topic_metadata_refresh_interval.count()));
  EXPECT_EQ(configuration->GetOption("metadata.max.age.ms"),
            std::to_string(default_producer.common.metadata_max_age.count()));
  EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
  EXPECT_EQ(configuration->GetOption("delivery.timeout.ms"),
            std::to_string(default_producer.delivery_timeout.count()));
  EXPECT_EQ(configuration->GetOption("queue.buffering.max.ms"),
            std::to_string(default_producer.queue_buffering_max.count()));
  EXPECT_EQ(configuration->GetOption("enable.idempotence"),
            default_producer.enable_idempotence ? "true" : "false");
  EXPECT_EQ(configuration->GetOption("queue.buffering.max.messages"),
            std::to_string(default_producer.queue_buffering_max_messages));
  EXPECT_EQ(configuration->GetOption("queue.buffering.max.kbytes"),
            std::to_string(default_producer.queue_buffering_max_kbytes));
  EXPECT_EQ(configuration->GetOption("message.max.bytes"),
            std::to_string(default_producer.message_max_bytes));
  EXPECT_EQ(configuration->GetOption("message.send.max.retries"),
            std::to_string(default_producer.message_send_max_retries));
  EXPECT_EQ(configuration->GetOption("retry.backoff.ms"),
            std::to_string(default_producer.retry_backoff.count()));
  EXPECT_EQ(configuration->GetOption("retry.backoff.max.ms"),
            std::to_string(default_producer.retry_backoff_max.count()));
}

UTEST_F(ConfigurationTest, ProducerNonDefault) {
  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.common.topic_metadata_refresh_interval = 10ms;
  producer_configuration.common.metadata_max_age = 30ms;
  producer_configuration.common.client_id = "test-client";
  producer_configuration.delivery_timeout = 37ms;
  producer_configuration.queue_buffering_max = 7ms;
  producer_configuration.enable_idempotence = true;
  producer_configuration.queue_buffering_max_messages = 33;
  producer_configuration.queue_buffering_max_kbytes = 55;
  producer_configuration.message_max_bytes = 3333;
  producer_configuration.message_send_max_retries = 3;
  producer_configuration.retry_backoff = 200ms;
  producer_configuration.retry_backoff_max = 2000ms;
  producer_configuration.rd_kafka_options["session.timeout.ms"] = "3600000";

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(
      MakeProducerConfiguration("kafka-producer", producer_configuration)));

  EXPECT_EQ(configuration->GetOption("client.id"), "test-client");
  EXPECT_EQ(configuration->GetOption("topic.metadata.refresh.interval.ms"),
            "10");
  EXPECT_EQ(configuration->GetOption("metadata.max.age.ms"), "30");
  EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
  EXPECT_EQ(configuration->GetOption("delivery.timeout.ms"), "37");
  EXPECT_EQ(configuration->GetOption("queue.buffering.max.ms"), "7");
  EXPECT_EQ(configuration->GetOption("enable.idempotence"), "true");
  EXPECT_EQ(configuration->GetOption("queue.buffering.max.messages"), "33");
  EXPECT_EQ(configuration->GetOption("queue.buffering.max.kbytes"), "55");
  EXPECT_EQ(configuration->GetOption("message.max.bytes"), "3333");
  EXPECT_EQ(configuration->GetOption("message.send.max.retries"), "3");
  EXPECT_EQ(configuration->GetOption("retry.backoff.ms"), "200");
  EXPECT_EQ(configuration->GetOption("retry.backoff.max.ms"), "2000");
  EXPECT_EQ(configuration->GetOption("session.timeout.ms"), "3600000");
}

UTEST_F(ConfigurationTest, Consumer) {
  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(
      configuration.emplace(MakeConsumerConfiguration("kafka-consumer")));

  const kafka::impl::ConsumerConfiguration default_consumer{};
  EXPECT_EQ(configuration->GetOption("client.id"),
            default_consumer.common.client_id);
  EXPECT_EQ(
      configuration->GetOption("topic.metadata.refresh.interval.ms"),
      std::to_string(
          default_consumer.common.topic_metadata_refresh_interval.count()));
  EXPECT_EQ(configuration->GetOption("metadata.max.age.ms"),
            std::to_string(default_consumer.common.metadata_max_age.count()));
  EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
  EXPECT_EQ(configuration->GetOption("group.id"), "test-group");
  EXPECT_EQ(configuration->GetOption("auto.offset.reset"),
            default_consumer.auto_offset_reset);
  EXPECT_EQ(configuration->GetOption("enable.auto.commit"), "false");
}

UTEST_F(ConfigurationTest, ConsumerNonDefault) {
  kafka::impl::ConsumerConfiguration consumer_configuration{};
  consumer_configuration.common.topic_metadata_refresh_interval = 10ms;
  consumer_configuration.common.metadata_max_age = 30ms;
  consumer_configuration.common.client_id = "test-client";
  consumer_configuration.auto_offset_reset = "largest";
  consumer_configuration.rd_kafka_options["socket.keepalive.enable"] = "true";

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(
      MakeConsumerConfiguration("kafka-consumer", consumer_configuration)));

  EXPECT_EQ(configuration->GetOption("topic.metadata.refresh.interval.ms"),
            "10");
  EXPECT_EQ(configuration->GetOption("client.id"), "test-client");
  EXPECT_EQ(configuration->GetOption("metadata.max.age.ms"), "30");
  EXPECT_EQ(configuration->GetOption("security.protocol"), "plaintext");
  EXPECT_EQ(configuration->GetOption("group.id"), "test-group");
  EXPECT_EQ(configuration->GetOption("auto.offset.reset"), "largest");
  EXPECT_EQ(configuration->GetOption("socket.keepalive.enable"), "true");
}

UTEST_F(ConfigurationTest, ProducerSecure) {
  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.security.security_protocol =
      kafka::impl::SecurityConfiguration::SaslSsl{
          /*security_mechanism=*/"SCRAM-SHA-512",
          /*ssl_ca_location=*/"probe"};

  kafka::impl::Secret secrets;
  secrets.username = kafka::impl::Secret::SecretType{"username"};
  secrets.password = kafka::impl::Secret::SecretType{"password"};

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration(
      "kafka-producer", producer_configuration, secrets)));

  EXPECT_EQ(configuration->GetOption("security.protocol"), "sasl_ssl");
  EXPECT_EQ(configuration->GetOption("sasl.mechanism"), "SCRAM-SHA-512");
  EXPECT_EQ(configuration->GetOption("sasl.username"), "username");
  EXPECT_EQ(configuration->GetOption("sasl.password"), "password");
  EXPECT_EQ(configuration->GetOption("ssl.ca.location"), "probe");
}

UTEST_F(ConfigurationTest, ConsumerSecure) {
  kafka::impl::ConsumerConfiguration consumer_configuration{};
  consumer_configuration.security.security_protocol =
      kafka::impl::SecurityConfiguration::SaslSsl{
          /*security_mechanism=*/"SCRAM-SHA-512",
          /*ssl_ca_location=*/"/etc/ssl/cert.ca"};

  kafka::impl::Secret secrets;
  secrets.username = kafka::impl::Secret::SecretType{"username"};
  secrets.password = kafka::impl::Secret::SecretType{"password"};

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration(
      "kafka-consumer", consumer_configuration, secrets)));

  EXPECT_EQ(configuration->GetOption("security.protocol"), "sasl_ssl");
  EXPECT_EQ(configuration->GetOption("sasl.mechanism"), "SCRAM-SHA-512");
  EXPECT_EQ(configuration->GetOption("sasl.username"), "username");
  EXPECT_EQ(configuration->GetOption("sasl.password"), "password");
  EXPECT_EQ(configuration->GetOption("ssl.ca.location"), "/etc/ssl/cert.ca");
}

UTEST_F(ConfigurationTest, IncorrectComponentName) {
  UEXPECT_THROW(MakeProducerConfiguration("producer"), std::runtime_error);
  UEXPECT_THROW(MakeConsumerConfiguration("consumer"), std::runtime_error);
}

UTEST_F(ConfigurationTest, ConsumerResolveGroupId) {
  kafka::impl::ConsumerConfiguration consumer_configuration{};
  consumer_configuration.group_id = "test-group-{pod_name}";
  consumer_configuration.env_pod_name = "ENVIRONMENT_VARIABLE_NAME";

  engine::subprocess::EnvironmentVariablesScope scope{};
  engine::subprocess::SetEnvironmentVariable(
      "ENVIRONMENT_VARIABLE_NAME", "pod-example-com",
      engine::subprocess::Overwrite::kAllowed);

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(
      MakeConsumerConfiguration("kafka-consumer", consumer_configuration)));

  EXPECT_EQ(configuration->GetOption("group.id"), "test-group-pod-example-com");
}

USERVER_NAMESPACE_END
