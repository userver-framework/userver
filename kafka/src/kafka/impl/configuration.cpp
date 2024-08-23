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

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/log_level.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

constexpr std::string_view kPlainTextProtocol = "PLAINTEXT";
constexpr std::string_view kSaslSSLProtocol = "SASL_SSL";
constexpr std::array kSupportedSecurityProtocols{kPlainTextProtocol,
                                                 kSaslSSLProtocol};
constexpr std::array kSupportedSaslSecurityMechanisms{"PLAIN", "SCRAM-SHA-512"};

constexpr std::string_view kSecurityProtocolBrokersField = "security_protocol";
constexpr std::string_view kSaslMechanismsBrokersField = "sasl_mechanisms";
constexpr std::string_view kSslCaLocationField = "ssl_ca_location";
constexpr std::string_view kTopicMetadataRefreshIntervalMsField =
    "topic_metadata_refresh_interval";
constexpr std::string_view kMetadataMaxAgeMsField = "metadata_max_age";

// Consumer specific fields
constexpr std::string_view kGroupIdField = "group_id";
constexpr std::string_view kEnvPodNameField = "env_pod_name";
constexpr std::string_view kEnableAutoCommitField = "enable_auto_commit";
constexpr std::string_view kAutoOffsetResetField = "auto_offset_reset";

constexpr std::string_view kPodNameSubstr = "{pod_name}";

// Producer specific fields
constexpr std::string_view kDeliveryTimeoutField = "delivery_timeout";
constexpr std::string_view kQueueBufferingMaxMsField = "queue_buffering_max";
constexpr std::string_view kEnableIdempotenceField = "enable_idempotence";

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

void LogCallback([[maybe_unused]] const rd_kafka_t* kafka_entity, int log_level,
                 const char* facility, const char* message) {
  LOG(convertRdKafkaLogLevelToLoggingLevel(log_level))
      << logging::LogExtra{{{"kafka_callback", "log_callback"},
                            {"facility", facility}}}
      << message;
}

}  // namespace

CommonConfiguration Parse(const yaml_config::YamlConfig& config,
                          formats::parse::To<CommonConfiguration>) {
  CommonConfiguration common{};
  common.topic_metadata_refresh_interval =
      config[kTopicMetadataRefreshIntervalMsField]
          .As<std::chrono::milliseconds>(
              common.topic_metadata_refresh_interval),
  common.metadata_max_age =
      config[kMetadataMaxAgeMsField].As<std::chrono::milliseconds>(
          common.metadata_max_age);

  return common;
};

SecurityConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<SecurityConfiguration>) {
  SecurityConfiguration security{};

  const auto protocol = config[kSecurityProtocolBrokersField].As<std::string>();
  if (!IsSupportedOption(kSupportedSecurityProtocols, protocol)) {
    ThrowUnsupportedOption("security protocol", protocol,
                           kSupportedSecurityProtocols);
  }
  if (protocol == kPlainTextProtocol) {
    security.security_protocol.emplace<SecurityConfiguration::Plaintext>();
  } else if (protocol == kSaslSSLProtocol) {
    const auto mechanism =
        config[kSaslMechanismsBrokersField].As<std::string>();
    if (!IsSupportedOption(kSupportedSaslSecurityMechanisms, mechanism)) {
      ThrowUnsupportedOption("SASL security mechanism", mechanism,
                             kSupportedSaslSecurityMechanisms);
    }

    security.security_protocol.emplace<SecurityConfiguration::SaslSsl>(
        SecurityConfiguration::SaslSsl{
            /*security_mechanism=*/mechanism,
            /*ssl_ca_location=*/config[kSslCaLocationField].As<std::string>()});
  }

  return security;
}

ConsumerConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<ConsumerConfiguration>) {
  ConsumerConfiguration consumer{};
  consumer.common = config.As<CommonConfiguration>();
  consumer.security = config.As<SecurityConfiguration>();
  consumer.group_id = config[kGroupIdField].As<std::string>();
  consumer.auto_offset_reset =
      config[kAutoOffsetResetField].As<std::string>(consumer.auto_offset_reset);
  consumer.enable_auto_commit =
      config[kEnableAutoCommitField].As<bool>(consumer.enable_auto_commit);
  if (config.HasMember(kEnvPodNameField)) {
    consumer.env_pod_name = config[kEnvPodNameField].As<std::string>();
  }

  return consumer;
}

ProducerConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<ProducerConfiguration>) {
  ProducerConfiguration producer{};
  producer.common = config.As<CommonConfiguration>();
  producer.security = config.As<SecurityConfiguration>(),
  producer.delivery_timeout =
      config[kDeliveryTimeoutField].As<std::chrono::milliseconds>(
          producer.delivery_timeout);
  producer.queue_buffering_max =
      config[kQueueBufferingMaxMsField].As<std::chrono::milliseconds>(
          producer.queue_buffering_max);
  producer.enable_idempotence =
      config[kEnableIdempotenceField].As<bool>(producer.enable_idempotence);

  return producer;
}

Configuration::Configuration(const std::string& name,
                             const ConsumerConfiguration& configuration,
                             const Secret& secrets)
    : name_(name), conf_(rd_kafka_conf_new(), &rd_kafka_conf_destroy) {
  VerifyComponentNamePrefix(name, "kafka-consumer");

  SetCommon(configuration.common);
  SetSecurity(configuration.security, secrets);
  SetConsumer(configuration);
}

Configuration::Configuration(const std::string& name,
                             const ProducerConfiguration& configuration,
                             const Secret& secrets)
    : name_(name), conf_(rd_kafka_conf_new(), &rd_kafka_conf_destroy) {
  VerifyComponentNamePrefix(name, "kafka-producer");

  SetCommon(configuration.common);
  SetSecurity(configuration.security, secrets);
  SetProducer(configuration);
}

const std::string& Configuration::GetName() const { return name_; }

rd_kafka_conf_s* Configuration::Release() && { return conf_.release(); }

void Configuration::SetCommon(const CommonConfiguration& common) {
  SetOption("topic.metadata.refresh.interval.ms",
            std::to_string(common.topic_metadata_refresh_interval.count()));
  SetOption("metadata.max.age.ms",
            std::to_string(common.metadata_max_age.count()));

  rd_kafka_conf_set_log_cb(conf_.get(), &LogCallback);
}

void Configuration::SetSecurity(const SecurityConfiguration& security,
                                const Secret& secrets) {
  SetOption("bootstrap.servers", secrets.brokers.c_str());

  utils::Visit(
      security.security_protocol,
      [](const SecurityConfiguration::Plaintext&) {
        LOG_INFO() << "Using PLAINTEXT security protocol";
      },
      [this, &secrets](const SecurityConfiguration::SaslSsl& sasl_ssl) {
        LOG_INFO() << "Using SASL_SSL security protocol";

        SetOption("security.protocol", std::string{kSaslSSLProtocol});
        SetOption("sasl.mechanism", sasl_ssl.security_mechanism);
        SetOption("sasl.username", secrets.username);
        SetOption("sasl.password", secrets.password.GetUnderlying());
        SetOption("ssl.ca.location", sasl_ssl.ssl_ca_location);
      });
}  // namespace kafka::impl

void Configuration::SetConsumer(const ConsumerConfiguration& configuration) {
  const auto group_id = ResolveGroupId(configuration);
  UINVARIANT(!group_id.empty(), "Consumer group_id must not be empty");

  LOG_INFO() << fmt::format("Consumer '{}' is going to join group '{}'", name_,
                            group_id);

  SetOption("group.id", group_id);
  SetOption("enable.auto.commit",
            configuration.enable_auto_commit ? "true" : "false");
  SetOption("auto.offset.reset", configuration.auto_offset_reset);
}

void Configuration::SetProducer(const ProducerConfiguration& configuration) {
  SetOption("delivery.timeout.ms",
            std::to_string(configuration.delivery_timeout.count()));
  SetOption("queue.buffering.max.ms",
            std::to_string(configuration.queue_buffering_max.count()));
  SetOption("enable.idempotence",
            configuration.enable_idempotence ? "true" : "false");
}

void Configuration::SetOption(const char* option, const char* value) {
  UASSERT(conf_);
  UASSERT(option);
  UASSERT(value);

  ErrorBuffer err_buf;
  const rd_kafka_conf_res_t err = rd_kafka_conf_set(
      conf_.get(), option, value, err_buf.data(), err_buf.size());
  if (err == RD_KAFKA_CONF_OK) {
    LOG_INFO() << fmt::format("Kafka conf option: '{}' -> '{}'", option, value);
    return;
  }

  PrintErrorAndThrow("set config option", err_buf);
}

void Configuration::SetOption(const char* option, const std::string& value) {
  SetOption(option, value.data());
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
