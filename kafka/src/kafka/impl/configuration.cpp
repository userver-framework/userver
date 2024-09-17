#include <kafka/impl/configuration.hpp>

#include <array>
#include <string_view>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <kafka/impl/error_buffer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

template <class SupportedList>
bool IsSupportedOption(const SupportedList& supported_options,
                       const std::string& configured_option) {
  return utils::ContainsIf(
      supported_options,
      [&configured_option](std::string_view supported_option) {
        return configured_option.compare(supported_option) == 0;
      });
}

template <class SupportedList>
[[noreturn]] void ThrowUnsupportedOption(
    std::string_view option, const std::string& configured_option,
    const SupportedList& supported_options) {
  throw std::runtime_error{
      fmt::format("Unsupported {} '{}'. Expected on of: [{}]", option,
                  configured_option, fmt::join(supported_options, ", "))};
}

void VerifyComponentNamePrefix(const std::string& component_name,
                               const std::string& expected_prefix) {
  // producer's component should start with kafka-producer, consumer's - with
  // kafka-consumer
  if (component_name.rfind(expected_prefix) != 0) {
    throw std::runtime_error{
        fmt::format("Component '{}' doesn't start with '{}'", component_name,
                    expected_prefix)};
  }
}

std::string ResolvePodName(const std::string& env_pod_name) {
  engine::subprocess::UpdateCurrentEnvironmentVariables();
  const auto env_variables =
      engine::subprocess::GetCurrentEnvironmentVariablesPtr();
  if (auto pod_name = env_variables->GetValueOptional(env_pod_name)) {
    return *pod_name;
  }

  throw std::runtime_error{
      fmt::format("Not found '{}' in env variables", env_pod_name)};
}

std::string ResolveGroupId(const ConsumerConfiguration& configuration) {
  static constexpr std::string_view kPodNameSubstr{"{pod_name}"};

  auto group_id{configuration.group_id};

  if (!configuration.env_pod_name.has_value()) {
    return group_id;
  }

  const auto pos = group_id.find(kPodNameSubstr);
  if (group_id.find(kPodNameSubstr) != std::string::npos) {
    const auto pod_name = ResolvePodName(*configuration.env_pod_name);

    return group_id.replace(pos, kPodNameSubstr.size(), pod_name);
  }

  return group_id;
}

}  // namespace

CommonConfiguration Parse(const yaml_config::YamlConfig& config,
                          formats::parse::To<CommonConfiguration>) {
  CommonConfiguration common{};
  common.topic_metadata_refresh_interval =
      config["topic_metadata_refresh_interval"].As<std::chrono::milliseconds>(
          common.topic_metadata_refresh_interval),
  common.metadata_max_age =
      config["metadata_max_age"].As<std::chrono::milliseconds>(
          common.metadata_max_age);
  common.client_id = config["client_id"].As<std::string>("");

  return common;
}

SecurityConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<SecurityConfiguration>) {
  static constexpr std::string_view kPlainTextProtocol{"PLAINTEXT"};
  static constexpr std::string_view kSaslSSLProtocol{"SASL_SSL"};
  static constexpr std::array kSupportedSecurityProtocols{kPlainTextProtocol,
                                                          kSaslSSLProtocol};
  static constexpr std::array kSupportedSaslSecurityMechanisms{"PLAIN",
                                                               "SCRAM-SHA-512"};

  SecurityConfiguration security{};

  const auto protocol = config["security_protocol"].As<std::string>();
  if (!IsSupportedOption(kSupportedSecurityProtocols, protocol)) {
    ThrowUnsupportedOption("security protocol", protocol,
                           kSupportedSecurityProtocols);
  }
  if (protocol == kPlainTextProtocol) {
    security.security_protocol.emplace<SecurityConfiguration::Plaintext>();
  } else if (protocol == kSaslSSLProtocol) {
    const auto mechanism = config["sasl_mechanisms"].As<std::string>();
    if (!IsSupportedOption(kSupportedSaslSecurityMechanisms, mechanism)) {
      ThrowUnsupportedOption("SASL security mechanism", mechanism,
                             kSupportedSaslSecurityMechanisms);
    }

    security.security_protocol.emplace<SecurityConfiguration::SaslSsl>(
        SecurityConfiguration::SaslSsl{
            /*security_mechanism=*/mechanism,
            /*ssl_ca_location=*/config["ssl_ca_location"].As<std::string>()});
  }

  return security;
}

ConsumerConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<ConsumerConfiguration>) {
  static constexpr std::string_view kEnvPodNameField{"env_pod_name"};

  ConsumerConfiguration consumer{};
  consumer.common = config.As<CommonConfiguration>();
  consumer.security = config.As<SecurityConfiguration>();
  consumer.rd_kafka_options =
      config["rd_kafka_custom_options"].As<RdKafkaOptions>({});
  consumer.group_id = config["group_id"].As<std::string>();
  consumer.auto_offset_reset =
      config["auto_offset_reset"].As<std::string>(consumer.auto_offset_reset);
  consumer.max_callback_duration =
      config["max_callback_duration"].As<std::chrono::milliseconds>(
          consumer.max_callback_duration);
  if (config.HasMember(kEnvPodNameField)) {
    consumer.env_pod_name = config[kEnvPodNameField].As<std::string>();
  }

  return consumer;
}

ProducerConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<ProducerConfiguration>) {
  ProducerConfiguration producer{};
  producer.common = config.As<CommonConfiguration>();
  producer.security = config.As<SecurityConfiguration>();
  producer.rd_kafka_options =
      config["rd_kafka_custom_options"].As<RdKafkaOptions>({});
  producer.delivery_timeout =
      config["delivery_timeout"].As<std::chrono::milliseconds>(
          producer.delivery_timeout);
  producer.queue_buffering_max =
      config["queue_buffering_max"].As<std::chrono::milliseconds>(
          producer.queue_buffering_max);
  producer.enable_idempotence =
      config["enable_idempotence"].As<bool>(producer.enable_idempotence);
  producer.queue_buffering_max_messages =
      config["queue_buffering_max_messages"].As<std::uint32_t>(
          producer.queue_buffering_max_messages);
  producer.queue_buffering_max_kbytes =
      config["queue_buffering_max_kbytes"].As<std::uint32_t>(
          producer.queue_buffering_max_kbytes);
  producer.message_max_bytes =
      config["message_max_bytes"].As<std::uint32_t>(producer.message_max_bytes);
  producer.message_send_max_retries =
      config["message_send_max_retries"].As<std::uint32_t>(
          producer.message_send_max_retries);
  producer.retry_backoff =
      config["retry_backoff"].As<std::chrono::milliseconds>(
          producer.retry_backoff);
  producer.retry_backoff_max =
      config["retry_backoff_max"].As<std::chrono::milliseconds>(
          producer.retry_backoff_max);

  return producer;
}

Configuration::Configuration(const std::string& name,
                             const ConsumerConfiguration& configuration,
                             const Secret& secrets)
    : name_(name), conf_(rd_kafka_conf_new()) {
  VerifyComponentNamePrefix(name, "kafka-consumer");

  SetCommon(configuration.common);
  SetSecurity(configuration.security, secrets);
  SetRdKafka(configuration.rd_kafka_options);
  SetConsumer(configuration);
}

Configuration::Configuration(const std::string& name,
                             const ProducerConfiguration& configuration,
                             const Secret& secrets)
    : name_(name), conf_(rd_kafka_conf_new()) {
  VerifyComponentNamePrefix(name, "kafka-producer");

  SetCommon(configuration.common);
  SetSecurity(configuration.security, secrets);
  SetRdKafka(configuration.rd_kafka_options);
  SetProducer(configuration);
}

const std::string& Configuration::GetName() const { return name_; }

std::string Configuration::GetOption(const char* option) const {
  UINVARIANT(conf_.GetHandle(), "Null conf");

  std::size_t result_size{0};
  const auto get_size_status =
      rd_kafka_conf_get(conf_.GetHandle(), option, nullptr, &result_size);
  UINVARIANT(RD_KAFKA_CONF_OK == get_size_status,
             fmt::format("Failed to retrieve configuration option {}", option));
  UINVARIANT(
      result_size > 0,
      fmt::format("Got size=0 while retrieving config option {}", option));

  std::string result_data(result_size, '\0');
  const auto get_result_status = rd_kafka_conf_get(
      conf_.GetHandle(), option, result_data.data(), &result_size);
  UINVARIANT(RD_KAFKA_CONF_OK == get_result_status,
             fmt::format("Failed to retrieve configuration option {}", option));

  result_data.resize(result_data.size() - 1);
  return result_data;
}

ConfHolder Configuration::Release() && { return std::move(conf_); }

void Configuration::SetCommon(const CommonConfiguration& common) {
  SetOption("topic.metadata.refresh.interval.ms",
            std::to_string(common.topic_metadata_refresh_interval.count()));
  SetOption("metadata.max.age.ms",
            std::to_string(common.metadata_max_age.count()));
  if (!common.client_id.empty())
    SetOption("client.id", common.client_id);
}

void Configuration::SetSecurity(const SecurityConfiguration& security,
                                const Secret& secrets) {
  SetOption("bootstrap.servers", secrets.brokers);

  utils::Visit(
      security.security_protocol,
      [](const SecurityConfiguration::Plaintext&) {
        LOG_INFO() << "Using PLAINTEXT security protocol";
      },
      [this, &secrets](const SecurityConfiguration::SaslSsl& sasl_ssl) {
        LOG_INFO() << "Using SASL_SSL security protocol";

        SetOption("security.protocol", "SASL_SSL");
        SetOption("sasl.mechanism", sasl_ssl.security_mechanism);
        SetOption("sasl.username", secrets.username);
        SetOption("sasl.password", secrets.password);
        SetOption("ssl.ca.location", sasl_ssl.ssl_ca_location);
      });
}

void Configuration::SetRdKafka(const RdKafkaOptions& rd_kafka_options) {
  if (rd_kafka_options.empty()) {
    return;
  }

  for (const auto& [option, value] : rd_kafka_options) {
    LOG_WARNING() << fmt::format("Setting custom rdkafka option '{}'", option);
    SetOption(option.c_str(), value.c_str(), value);
  }
}

void Configuration::SetConsumer(const ConsumerConfiguration& configuration) {
  const auto group_id = ResolveGroupId(configuration);
  UINVARIANT(!group_id.empty(), "Consumer group_id must not be empty");

  LOG_INFO() << fmt::format("Consumer '{}' is going to join group '{}'", name_,
                            group_id);

  SetOption("group.id", group_id);
  SetOption("enable.auto.commit", "false");
  SetOption("auto.offset.reset", configuration.auto_offset_reset);
  SetOption("max.poll.interval.ms", configuration.max_callback_duration);
  rd_kafka_conf_set_events(conf_.GetHandle(),
                           RD_KAFKA_EVENT_LOG | RD_KAFKA_EVENT_ERROR |
                               RD_KAFKA_EVENT_OFFSET_COMMIT |
                               RD_KAFKA_EVENT_REBALANCE | RD_KAFKA_EVENT_FETCH);
}

void Configuration::SetProducer(const ProducerConfiguration& configuration) {
  SetOption("delivery.timeout.ms", configuration.delivery_timeout);
  SetOption("queue.buffering.max.ms", configuration.queue_buffering_max);
  SetOption("enable.idempotence",
            configuration.enable_idempotence ? "true" : "false");
  SetOption("queue.buffering.max.messages",
            configuration.queue_buffering_max_messages);
  SetOption("queue.buffering.max.kbytes",
            configuration.queue_buffering_max_kbytes);
  SetOption("message.max.bytes", configuration.message_max_bytes);
  SetOption("message.send.max.retries", configuration.message_send_max_retries);
  SetOption("retry.backoff.ms", configuration.retry_backoff);
  SetOption("retry.backoff.max.ms", configuration.retry_backoff_max);
  rd_kafka_conf_set_events(conf_.GetHandle(), RD_KAFKA_EVENT_LOG |
                                                  RD_KAFKA_EVENT_ERROR |
                                                  RD_KAFKA_EVENT_DR);
}

template <class T>
void Configuration::SetOption(const char* option, const char* value,
                              T to_print) {
  UASSERT(conf_.GetHandle());
  UASSERT(option);
  UASSERT(value);

  ErrorBuffer err_buf;
  const rd_kafka_conf_res_t err = rd_kafka_conf_set(
      conf_.GetHandle(), option, value, err_buf.data(), err_buf.size());
  if (err == RD_KAFKA_CONF_OK) {
    LOG_INFO() << fmt::format("Kafka conf option: '{}' -> '{}'", option,
                              to_print);
    return;
  }

  PrintErrorAndThrow("set config option", err_buf);
}

void Configuration::SetOption(const char* option, const std::string& value) {
  SetOption(option, value.c_str(), value);
}

void Configuration::SetOption(const char* option,
                              std::chrono::milliseconds value) {
  SetOption(option, std::to_string(value.count()).c_str(),
            fmt::format("{}ms", value.count()));
}

void Configuration::SetOption(const char* option, std::uint32_t value) {
  SetOption(option, std::to_string(value).c_str(), value);
}

void Configuration::SetOption(const char* option,
                              const Secret::SecretType& value) {
  SetOption(option, value.GetUnderlying().c_str(), "*****");
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
