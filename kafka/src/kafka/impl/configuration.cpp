#include <kafka/impl/configuration.hpp>

#include <array>
#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/algo.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/log_level.hpp>
#include <kafka/impl/opaque.hpp>
#include <kafka/impl/stats.hpp>

#include <librdkafka/rdkafka.h>

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

void HandleConfSetError(rd_kafka_conf_res_t err, const ErrorBuffer& err_buf) {
  if (err == RD_KAFKA_CONF_OK) {
    return;
  }

  PrintErrorAndThrow("set config option", err_buf);
}

void LogCallback([[maybe_unused]] const rd_kafka_t* kafka_entity, int log_level,
                 const char* facility, const char* message) {
  LOG(convertRdKafkaLogLevelToLoggingLevel(log_level))
      << logging::LogExtra{{{"kafka_callback", "log_callback"},
                            {"facility", facility}}}
      << message;
}

void ErrorCallback([[maybe_unused]] rd_kafka_t* kafka_entity, int error_code,
                   const char* reason, void* opaque_ptr) {
  const auto& opaque = Opaque::FromPtr(opaque_ptr);
  const auto log_tags = opaque.MakeCallbackLogTags("error_callback");

  LOG_ERROR() << log_tags
              << fmt::format("Error {} occured because of '{}': {}", error_code,
                             reason,
                             rd_kafka_err2str(
                                 static_cast<rd_kafka_resp_err_t>(error_code)));

  if (error_code == RD_KAFKA_RESP_ERR__RESOLVE ||
      error_code == RD_KAFKA_RESP_ERR__TRANSPORT ||
      error_code == RD_KAFKA_RESP_ERR__AUTHENTICATION ||
      error_code == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
    ++opaque.GetStats().connections_error;
  }
}

}  // namespace

class Configuration::ConfHolder final {
 public:
  ConfHolder() : handle_(rd_kafka_conf_new(), &rd_kafka_conf_destroy) {}

  ~ConfHolder() = default;

  ConfHolder(const ConfHolder&) = delete;
  ConfHolder& operator=(const ConfHolder&) = delete;

  ConfHolder(ConfHolder&& other) noexcept : handle_(std::move(other.handle_)) {}
  ConfHolder& operator=(ConfHolder&&) noexcept = delete;

  rd_kafka_conf_t* Handle() { return handle_.get(); }

  rd_kafka_conf_t* Release() { return handle_.release(); }

 private:
  using HandleHolder =
      std::unique_ptr<rd_kafka_conf_t, decltype(&rd_kafka_conf_destroy)>;

  HandleHolder handle_;
};

Configuration::Configuration(const components::ComponentConfig& config,
                             const components::ComponentContext& context,
                             EntityType entity_type)
    : component_name_(config.Name()), conf_(std::make_unique<ConfHolder>()) {
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

Configuration::~Configuration() = default;

Configuration::Configuration(Configuration&& other) noexcept = default;

const std::string& Configuration::GetComponentName() const {
  return component_name_;
}

Configuration& Configuration::SetOpaque(Opaque& opaque) {
  rd_kafka_conf_set_opaque(conf_->Handle(), &opaque);

  return *this;
}

Configuration& Configuration::SetCallbacks(CallbacksSetter callback_setter) {
  callback_setter(conf_->Handle());

  return *this;
}

void* Configuration::Release() { return conf_->Release(); }

void Configuration::SetCommonOptions(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  const auto& broker_secrets =
      context.FindComponent<components::Secdist>().Get().Get<BrokerSecrets>();

  const auto& secrets = broker_secrets.GetSecretByComponentName(config.Name());

  ErrorBuffer err_buf{};

  HandleConfSetError(rd_kafka_conf_set(conf_->Handle(), "bootstrap.servers",
                                       secrets.brokers.c_str(), err_buf.data(),
                                       err_buf.size()),
                     err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(conf_->Handle(), "topic.metadata.refresh.interval.ms",
                        config[kTopicMetadataRefreshIntervalMsField]
                            .As<std::string>("300000")
                            .c_str(),
                        err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(
          conf_->Handle(), "metadata.max.age.ms",
          config[kMetadataMaxAgeMsField].As<std::string>("900000").c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);

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

    HandleConfSetError(rd_kafka_conf_set(conf_->Handle(), "security.protocol",
                                         securityProtocol.c_str(),
                                         err_buf.data(), err_buf.size()),
                       err_buf);
    HandleConfSetError(rd_kafka_conf_set(conf_->Handle(), "sasl.mechanism",
                                         securityMechanism.c_str(),
                                         err_buf.data(), err_buf.size()),
                       err_buf);
    HandleConfSetError(rd_kafka_conf_set(conf_->Handle(), "sasl.username",
                                         secrets.username.c_str(),
                                         err_buf.data(), err_buf.size()),
                       err_buf);
    HandleConfSetError(
        rd_kafka_conf_set(conf_->Handle(), "sasl.password",
                          secrets.password.GetUnderlying().c_str(),
                          err_buf.data(), err_buf.size()),
        err_buf);
    HandleConfSetError(
        rd_kafka_conf_set(conf_->Handle(), "ssl.ca.location",
                          config[kSslCaLocationField].As<std::string>().c_str(),
                          err_buf.data(), err_buf.size()),
        err_buf);
  }

  rd_kafka_conf_set_log_cb(conf_->Handle(), &LogCallback);
  rd_kafka_conf_set_error_cb(conf_->Handle(), &ErrorCallback);
}

void Configuration::SetConsumerOptions(
    const components::ComponentConfig& config) {
  VerifyComponentNamePrefix(config.Name(), "kafka-consumer");

  ErrorBuffer err_buf{};

  const auto group_id = ResolveGroupId(config);
  LOG_INFO() << fmt::format("Consumer '{}' is going to join group '{}'",
                            config.Name(), group_id);

  HandleConfSetError(
      rd_kafka_conf_set(conf_->Handle(), "group.id", group_id.c_str(),
                        err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(
          conf_->Handle(), "enable.auto.commit",
          config[kEnableAutoCommitField].As<std::string>("false").c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(conf_->Handle(), "auto.offset.reset",
                        config[kAutoOffsetResetField].As<std::string>().c_str(),
                        err_buf.data(), err_buf.size()),
      err_buf);
}

void Configuration::SetProducerOptions(
    const components::ComponentConfig& config) {
  VerifyComponentNamePrefix(config.Name(), "kafka-producer");

  ErrorBuffer err_buf{};

  HandleConfSetError(
      rd_kafka_conf_set(conf_->Handle(), "delivery.timeout.ms",
                        config[kDeliveryTimeoutField].As<std::string>().c_str(),
                        err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(
          conf_->Handle(), "queue.buffering.max.ms",
          config[kQueueBufferingMaxMsField].As<std::string>().c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(
          conf_->Handle(), "enable.idempotence",
          config[kEnableIdempotenceField].As<std::string>("false").c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
