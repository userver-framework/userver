#include <kafka/impl/configuration.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/log_level.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/algo.hpp>

#include <librdkafka/rdkafka.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <array>

namespace kafka {

namespace {

constexpr std::string_view kPlainTextProtocol = "PLAINTEXT";
constexpr std::string_view kSaslSSLProtocol = "SASL_SSL";
constexpr std::array kSupportedSecurityProtocols{kPlainTextProtocol,
                                                 kSaslSSLProtocol};
constexpr std::array kSupportedSaslSecurityMechanisms{"PLAIN", "SCRAM-SHA-512"};

constexpr std::string_view kSecurityProtocolBrokersField = "security_protocol";
constexpr std::string_view kSaslMechanismsBrokersField = "sasl_mechanisms";
constexpr std::string_view kSslCaLocationField = "ssl_ca_location";

// Producer specific fields
constexpr std::string_view kDeliveryTimeoutField = "delivery_timeout_ms";
constexpr std::string_view kQueueBufferingMaxMsField = "queue_buffering_max_ms";
constexpr std::string_view kEnableIdempotenceField = "enable_idempotence";

using ConfHolder =
    std::unique_ptr<rd_kafka_conf_t, decltype(&rd_kafka_conf_destroy)>;

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

void HandleConfSetError(rd_kafka_conf_res_t err,
                        const impl::ErrorBuffer& err_buf) {
  if (err == RD_KAFKA_CONF_OK) {
    return;
  }

  impl::PrintErrorAndThrow("set config option", err_buf);
}

namespace callbacks {

void LogCallback([[maybe_unused]] const rd_kafka_t* kafka_object, int log_level,
                 const char* facility, const char* message) {
  LOG(impl::convertRdKafkaLogLevelToLoggingLevel(log_level))
      << logging::LogExtra{{{"kafka_callback", "log_callback"},
                            {"facility", facility}}}
      << message;
}

void ErrorCallback([[maybe_unused]] rd_kafka_t* kafka_object, int error_code,
                   const char* reason, void* opaque) {
  auto* stats_and_logging_ = static_cast<impl::StatsAndLogging*>(opaque);
  auto log_tags = stats_and_logging_->log_tags_;
  log_tags.Extend({"kafka_callback", "error_callback"});

  LOG_ERROR() << log_tags
              << fmt::format("Error {} occured because of '{}': {}", error_code,
                             reason,
                             rd_kafka_err2str(
                                 static_cast<rd_kafka_resp_err_t>(error_code)));

  if (error_code == RD_KAFKA_RESP_ERR__RESOLVE ||
      error_code == RD_KAFKA_RESP_ERR__TRANSPORT ||
      error_code == RD_KAFKA_RESP_ERR__AUTHENTICATION ||
      error_code == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
    ++stats_and_logging_->stats->connections_error;
  }
}

namespace producer {

/// @param message represents the delivered (or not) message. Its `_private`
/// field contains for `opaque` argument, which was passed to
/// `rd_kafka_producev`
/// @param opaque A global opaque parameter which is equal for all callbacks and
/// is set with `rd_kafka_conf_set_opaque`
void DeliveryReportCallback([[maybe_unused]] rd_kafka_t* producer_,
                            const rd_kafka_message_t* message,
                            [[maybe_unused]] void* opaque) {
  auto* complete_handle = static_cast<impl::DeliveryWaiter*>(message->_private);
  complete_handle->set_value(message->err);
  delete complete_handle;

  const char* topic_name = rd_kafka_topic_name(message->rkt);

  auto* stats_and_logging_ = static_cast<impl::StatsAndLogging*>(opaque);
  auto& topic_stats = stats_and_logging_->stats->topics_stats[topic_name];
  auto log_tags = stats_and_logging_->log_tags_;
  log_tags.Extend({"kafka_callback", "delivery_callback"});

  if (message->err) {
    ++topic_stats->messages_counts.messages_error;
    LOG_WARNING() << log_tags
                  << fmt::format("Failed to delivery message to topic '{}': {}",
                                 topic_name, rd_kafka_err2str(message->err));
    return;
  }

  ++topic_stats->messages_counts.messages_success;
  LOG_DEBUG() << log_tags
              << fmt::format(
                     "Message to topic '{}' delivered successfully to "
                     "partition "
                     "{} by offset {}",
                     topic_name, message->partition, message->offset);
}

}  // namespace producer

namespace consumer {}  // namespace consumer

}  // namespace callbacks

ConfHolder MakeCommonConfiguration(const components::ComponentConfig& config,
                                   const impl::BrokerSecrets& broker_secrets,
                                   impl::StatsAndLogging* stats_and_logging,
                                   impl::ErrorBuffer& err_buf) {
  ConfHolder conf{rd_kafka_conf_new(), &rd_kafka_conf_destroy};

  const auto& secrets = broker_secrets.GetSecretByComponentName(config.Name());

  HandleConfSetError(rd_kafka_conf_set(conf.get(), "bootstrap.servers",
                                       secrets.brokers.c_str(), err_buf.data(),
                                       err_buf.size()),
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

    HandleConfSetError(rd_kafka_conf_set(conf.get(), "security.protocol",
                                         securityProtocol.c_str(),
                                         err_buf.data(), err_buf.size()),
                       err_buf);
    HandleConfSetError(rd_kafka_conf_set(conf.get(), "sasl.mechanism",
                                         securityMechanism.c_str(),
                                         err_buf.data(), err_buf.size()),
                       err_buf);
    HandleConfSetError(
        rd_kafka_conf_set(conf.get(), "sasl.username", secrets.username.c_str(),
                          err_buf.data(), err_buf.size()),
        err_buf);
    HandleConfSetError(
        rd_kafka_conf_set(conf.get(), "sasl.password",
                          secrets.password.GetUnderlying().c_str(),
                          err_buf.data(), err_buf.size()),
        err_buf);
    HandleConfSetError(
        rd_kafka_conf_set(conf.get(), "ssl.ca.location",
                          config[kSslCaLocationField].As<std::string>().c_str(),
                          err_buf.data(), err_buf.size()),
        err_buf);
  }

  /// @note passed pointer will be available in each callback as @param opaque
  rd_kafka_conf_set_opaque(conf.get(), stats_and_logging);
  rd_kafka_conf_set_log_cb(conf.get(), callbacks::LogCallback);
  rd_kafka_conf_set_error_cb(conf.get(), callbacks::ErrorCallback);

  return conf;
}

}  // namespace

Configuration::Configuration(const components::ComponentConfig& config,
                             const components::ComponentContext& context)
    : config_(config),
      broker_secrets_(context.FindComponent<components::Secdist>()
                          .Get()
                          .Get<impl::BrokerSecrets>()) {}

std::unique_ptr<ProducerImpl> Configuration::MakeProducerImpl(
    const logging::LogExtra& log_tags) const {
  VerifyComponentNamePrefix(config_.Name(), "kafka-producer");

  impl::ErrorBuffer err_buf{};

  auto stats_and_logging = std::make_unique<impl::StatsAndLogging>(
      log_tags, std::make_unique<impl::Stats>());
  auto conf = MakeCommonConfiguration(config_, broker_secrets_,
                                      stats_and_logging.get(), err_buf);

  HandleConfSetError(
      rd_kafka_conf_set(
          conf.get(), "delivery.timeout.ms",
          config_[kDeliveryTimeoutField].As<std::string>().c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(
          conf.get(), "queue.buffering.max.ms",
          config_[kQueueBufferingMaxMsField].As<std::string>().c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);
  HandleConfSetError(
      rd_kafka_conf_set(
          conf.get(), "enable.idempotence",
          config_[kEnableIdempotenceField].As<std::string>("false").c_str(),
          err_buf.data(), err_buf.size()),
      err_buf);

  rd_kafka_conf_set_dr_msg_cb(conf.get(),
                              callbacks::producer::DeliveryReportCallback);

  /// @remark librdkafka takes ownership on conf when passing it to producer
  /// create function, hence it must be released and no
  /// `rd_kafka_conf_destroy` should be called
  return std::make_unique<ProducerImpl>(conf.release(),
                                        std::move(stats_and_logging));
}

}  // namespace kafka
