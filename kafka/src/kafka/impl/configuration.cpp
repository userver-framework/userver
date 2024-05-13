#include <kafka/impl/configuration.hpp>

#include <array>
#include <string_view>
#include <utility>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/algo.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/log_level.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

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
    "topic_metadata_refresh_interval_ms";
constexpr std::string_view kMetadataMaxAgeMsField = "metadata_max_age_ms";

// Consumer specific fields
constexpr std::string_view kGroupIdField = "group_id";
constexpr std::string_view kEnvPodNameField = "env_pod_name";
constexpr std::string_view kEnableAutoCommitField = "enable_auto_commit";
constexpr std::string_view kAutoOffsetResetField = "auto_offset_reset";

constexpr std::string_view kPodNameSubstr = "{pod_name}";

// Producer specific fields
constexpr std::string_view kDeliveryTimeoutField = "delivery_timeout_ms";
constexpr std::string_view kQueueBufferingMaxMsField = "queue_buffering_max_ms";
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

std::string ResolvePodName(const components::ComponentConfig& config) {
  if (!config.HasMember(kEnvPodNameField)) {
    throw std::runtime_error(
        fmt::format("Found '{}' in group id but not found '{}' in config",
                    kPodNameSubstr, kEnvPodNameField));
  }
  const auto env_pod_name = config[kEnvPodNameField].As<std::string>();
  const auto env_variables =
      engine::subprocess::GetCurrentEnvironmentVariablesPtr();
  if (auto pod_name = env_variables->GetValueOptional(env_pod_name)) {
    return *pod_name;
  }

  throw std::runtime_error{
      fmt::format("Not found '{}' in env variables", env_pod_name)};
}

std::string ResolveGroupId(const components::ComponentConfig& config) {
  auto group_id = config[kGroupIdField].As<std::string>();

  const auto pos = group_id.find(kPodNameSubstr);
  if (pos != std::string::npos) {
    const auto pod_name = ResolvePodName(config);

    return group_id.replace(pos, kPodNameSubstr.size(), pod_name);
  }

  return group_id;
}

void ConfSetOption(rd_kafka_conf_t* conf, const char* option,
                   const char* value) {
  UASSERT(option);
  UASSERT(value);

  ErrorBuffer err_buf;

  const rd_kafka_conf_res_t err =
      rd_kafka_conf_set(conf, option, value, err_buf.data(), err_buf.size());
  if (err == RD_KAFKA_CONF_OK) {
    LOG_INFO() << fmt::format("Kafka conf option: '{}' -> '{}'", option, value);
    return;
  }

  PrintErrorAndThrow("set config option", err_buf);
}

void ConfSetOption(rd_kafka_conf_t* conf, const char* option,
                   const std::string& value) {
  ConfSetOption(conf, option, value.c_str());
}

void LogCallback([[maybe_unused]] const rd_kafka_t* kafka_entity, int log_level,
                 const char* facility, const char* message) {
  LOG(convertRdKafkaLogLevelToLoggingLevel(log_level))
      << logging::LogExtra{{{"kafka_callback", "log_callback"},
                            {"facility", facility}}}
      << message;
}

}  // namespace

Configuration::Configuration(const components::ComponentConfig& config,
                             const components::ComponentContext& context,
                             EntityType entity_type)
    : component_name_(config.Name()), conf_(rd_kafka_conf_new()) {
  SetCommonOptions(config, context);

  switch (entity_type) {
    case EntityType::kConsumer:
      SetConsumerOptions(config);
      break;
    case EntityType::kProducer:
      SetProducerOptions(config);
      break;
  }
}

Configuration::~Configuration() {
  if (conf_ != nullptr) {
    rd_kafka_conf_destroy(conf_);
  }
}

Configuration::Configuration(Configuration&& other) noexcept
    : component_name_(std::move(other.component_name_)),
      conf_(std::exchange(other.conf_, nullptr)) {}

const std::string& Configuration::GetComponentName() const {
  return component_name_;
}

rd_kafka_conf_s* Configuration::Release() {
  return std::exchange(conf_, nullptr);
}

void Configuration::SetCommonOptions(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  const auto& broker_secrets =
      context.FindComponent<components::Secdist>().Get().Get<BrokerSecrets>();

  const auto& secrets = broker_secrets.GetSecretByComponentName(config.Name());

  ConfSetOption(conf_, "bootstrap.servers", secrets.brokers.c_str());
  ConfSetOption(
      conf_, "topic.metadata.refresh.interval.ms",
      config[kTopicMetadataRefreshIntervalMsField].As<std::string>("300000"));
  ConfSetOption(conf_, "metadata.max.age.ms",
                config[kMetadataMaxAgeMsField].As<std::string>("900000"));

  const auto& securityProtocol =
      config[kSecurityProtocolBrokersField].As<std::string>(kPlainTextProtocol);
  if (!IsSupportedOption(kSupportedSecurityProtocols, securityProtocol)) {
    ThrowUnsupportedOption("security protocol", securityProtocol,
                           kSupportedSecurityProtocols);
  }

  if (securityProtocol == kSaslSSLProtocol) {
    const auto& securityMechanism =
        config[kSaslMechanismsBrokersField].As<std::string>();

    if (!IsSupportedOption(kSupportedSaslSecurityMechanisms,
                           securityMechanism)) {
      ThrowUnsupportedOption("SASL security mechanism", securityMechanism,
                             kSupportedSaslSecurityMechanisms);
    }

    ConfSetOption(conf_, "security.protocol", securityProtocol);
    ConfSetOption(conf_, "sasl.mechanism", securityMechanism);
    ConfSetOption(conf_, "sasl.username", secrets.username);
    ConfSetOption(conf_, "sasl.password", secrets.password.GetUnderlying());
    ConfSetOption(conf_, "ssl.ca.location",
                  config[kSslCaLocationField].As<std::string>());
  }

  rd_kafka_conf_set_log_cb(conf_, &LogCallback);
}

void Configuration::SetConsumerOptions(
    const components::ComponentConfig& config) {
  VerifyComponentNamePrefix(config.Name(), "kafka-consumer");

  const auto group_id = ResolveGroupId(config);
  LOG_INFO() << fmt::format("Consumer '{}' is going to join group '{}'",
                            config.Name(), group_id);

  ConfSetOption(conf_, "group.id", group_id);
  ConfSetOption(conf_, "enable.auto.commit",
                config[kEnableAutoCommitField].As<std::string>("false"));
  ConfSetOption(conf_, "auto.offset.reset",
                config[kAutoOffsetResetField].As<std::string>());
}

void Configuration::SetProducerOptions(
    const components::ComponentConfig& config) {
  VerifyComponentNamePrefix(config.Name(), "kafka-producer");

  ConfSetOption(conf_, "delivery.timeout.ms",
                config[kDeliveryTimeoutField].As<std::string>());
  ConfSetOption(conf_, "queue.buffering.max.ms",
                config[kQueueBufferingMaxMsField].As<std::string>());
  ConfSetOption(conf_, "enable.idempotence",
                config[kEnableIdempotenceField].As<std::string>("false"));
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
