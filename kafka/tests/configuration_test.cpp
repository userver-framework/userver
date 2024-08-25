#include "test_utils.hpp"

#include <optional>
#include <vector>

#include <gmock/gmock.h>
#include <librdkafka/rdkafka.h>

#include <userver/engine/subprocess/environment_variables.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using namespace std::chrono_literals;

class ConfigurationTest : public KafkaCluster {};

using ConfHolder =
    std::unique_ptr<rd_kafka_conf_t, decltype(&rd_kafka_conf_destroy)>;

ConfHolder RetrieveConf(kafka::impl::Configuration&& configuration) {
  return ConfHolder{std::move(configuration).Release(), &rd_kafka_conf_destroy};
}

std::string GetConfOption(const ConfHolder& conf, const char* option) {
  UINVARIANT(conf, "Null conf");

  std::size_t result_size{0};
  const auto get_size_status =
      rd_kafka_conf_get(conf.get(), option, nullptr, &result_size);
  UINVARIANT(RD_KAFKA_CONF_OK == get_size_status,
             fmt::format("Failed to retrieve configuration option {}", option));
  UINVARIANT(
      result_size > 0,
      fmt::format("Got size=0 while retrieving config option {}", option));

  std::string result_data(result_size, '\0');
  const auto get_result_status =
      rd_kafka_conf_get(conf.get(), option, result_data.data(), &result_size);
  UINVARIANT(RD_KAFKA_CONF_OK == get_result_status,
             fmt::format("Failed to retrieve configuration option {}", option));

  result_data.resize(result_data.size() - 1);
  return result_data;
}

}  // namespace

UTEST_F(ConfigurationTest, Producer) {
  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(
      configuration.emplace(MakeProducerConfiguration("kafka-producer")));

  const auto conf = RetrieveConf(std::move(*configuration));
  const kafka::impl::ProducerConfiguration default_producer{};
  EXPECT_EQ(
      GetConfOption(conf, "topic.metadata.refresh.interval.ms"),
      std::to_string(
          default_producer.common.topic_metadata_refresh_interval.count()));
  EXPECT_EQ(GetConfOption(conf, "metadata.max.age.ms"),
            std::to_string(default_producer.common.metadata_max_age.count()));
  EXPECT_EQ(GetConfOption(conf, "security.protocol"), "plaintext");
  EXPECT_EQ(GetConfOption(conf, "delivery.timeout.ms"),
            std::to_string(default_producer.delivery_timeout.count()));
  EXPECT_EQ(GetConfOption(conf, "queue.buffering.max.ms"),
            std::to_string(default_producer.queue_buffering_max.count()));
  EXPECT_EQ(GetConfOption(conf, "enable.idempotence"),
            default_producer.enable_idempotence ? "true" : "false");
}

UTEST_F(ConfigurationTest, ProducerNonDefault) {
  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.common.topic_metadata_refresh_interval = 10ms;
  producer_configuration.common.metadata_max_age = 30ms;
  producer_configuration.delivery_timeout = 37ms;
  producer_configuration.queue_buffering_max = 7ms;
  producer_configuration.enable_idempotence = true;

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(
      MakeProducerConfiguration("kafka-producer", producer_configuration)));

  const auto conf = RetrieveConf(std::move(*configuration));
  const kafka::impl::ProducerConfiguration default_producer{};
  EXPECT_EQ(GetConfOption(conf, "topic.metadata.refresh.interval.ms"), "10");
  EXPECT_EQ(GetConfOption(conf, "metadata.max.age.ms"), "30");
  EXPECT_EQ(GetConfOption(conf, "security.protocol"), "plaintext");
  EXPECT_EQ(GetConfOption(conf, "delivery.timeout.ms"), "37");
  EXPECT_EQ(GetConfOption(conf, "queue.buffering.max.ms"), "7");
  EXPECT_EQ(GetConfOption(conf, "enable.idempotence"), "true");
}

UTEST_F(ConfigurationTest, Consumer) {
  kafka::impl::ConsumerConfiguration consumer_configuration{};
  consumer_configuration.common.topic_metadata_refresh_interval = 10ms;
  consumer_configuration.common.metadata_max_age = 30ms;
  consumer_configuration.auto_offset_reset = "largest";
  consumer_configuration.enable_auto_commit = true;

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(
      configuration.emplace(MakeConsumerConfiguration("kafka-consumer")));

  const auto conf = RetrieveConf(std::move(*configuration));
  const kafka::impl::ConsumerConfiguration default_consumer{};
  EXPECT_EQ(
      GetConfOption(conf, "topic.metadata.refresh.interval.ms"),
      std::to_string(
          default_consumer.common.topic_metadata_refresh_interval.count()));
  EXPECT_EQ(GetConfOption(conf, "metadata.max.age.ms"),
            std::to_string(default_consumer.common.metadata_max_age.count()));
  EXPECT_EQ(GetConfOption(conf, "security.protocol"), "plaintext");
  EXPECT_EQ(GetConfOption(conf, "group.id"), "test-group");
  EXPECT_EQ(GetConfOption(conf, "auto.offset.reset"),
            default_consumer.auto_offset_reset);
  EXPECT_EQ(GetConfOption(conf, "enable.auto.commit"),
            default_consumer.enable_auto_commit ? "true" : "false");
}

UTEST_F(ConfigurationTest, ProducerSecure) {
  kafka::impl::ProducerConfiguration producer_configuration{};
  producer_configuration.security.security_protocol =
      kafka::impl::SecurityConfiguration::SaslSsl{
          /*security_mechanism=*/"SCRAM-SHA-512",
          /*ssl_ca_location=*/"probe"};

  kafka::impl::Secret secrets;
  secrets.username = "username";
  secrets.password = kafka::impl::Secret::Password{"password"};

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(MakeProducerConfiguration(
      "kafka-producer", producer_configuration, secrets)));

  const auto conf = RetrieveConf(std::move(*configuration));
  const kafka::impl::SecurityConfiguration::SaslSsl default_sasl_ssl{};
  EXPECT_EQ(GetConfOption(conf, "security.protocol"), "sasl_ssl");
  EXPECT_EQ(GetConfOption(conf, "sasl.mechanism"), "SCRAM-SHA-512");
  EXPECT_EQ(GetConfOption(conf, "sasl.username"), "username");
  EXPECT_EQ(GetConfOption(conf, "sasl.password"), "password");
  EXPECT_EQ(GetConfOption(conf, "ssl.ca.location"), "probe");
}

UTEST_F(ConfigurationTest, ConsumerSecure) {
  kafka::impl::ConsumerConfiguration consumer_configuration{};
  consumer_configuration.security.security_protocol =
      kafka::impl::SecurityConfiguration::SaslSsl{
          /*security_mechanism=*/"SCRAM-SHA-512",
          /*ssl_ca_location=*/"/etc/ssl/cert.ca"};

  kafka::impl::Secret secrets;
  secrets.username = "username";
  secrets.password = kafka::impl::Secret::Password{"password"};

  std::optional<kafka::impl::Configuration> configuration;
  UEXPECT_NO_THROW(configuration.emplace(MakeConsumerConfiguration(
      "kafka-consumer", consumer_configuration, secrets)));

  const auto conf = RetrieveConf(std::move(*configuration));
  const kafka::impl::SecurityConfiguration::SaslSsl default_sasl_ssl{};
  EXPECT_EQ(GetConfOption(conf, "security.protocol"), "sasl_ssl");
  EXPECT_EQ(GetConfOption(conf, "sasl.mechanism"), "SCRAM-SHA-512");
  EXPECT_EQ(GetConfOption(conf, "sasl.username"), "username");
  EXPECT_EQ(GetConfOption(conf, "sasl.password"), "password");
  EXPECT_EQ(GetConfOption(conf, "ssl.ca.location"), "/etc/ssl/cert.ca");
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

  const auto conf = RetrieveConf(std::move(*configuration));
  EXPECT_EQ(GetConfOption(conf, "group.id"), "test-group-pod-example-com");
}

USERVER_NAMESPACE_END
