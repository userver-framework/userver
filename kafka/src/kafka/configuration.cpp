#include <kafka/configuration.hpp>

#include <cppkafka/configuration.h>
#include <cppkafka/logging.h>
#include <fmt/format.h>

#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>

#include <kafka/impl/log_level.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace {

constexpr std::string_view kSaslMechanismsBrokersField = "sasl_mechanisms";
constexpr std::string_view kSecurityProtocolBrokersField = "security_protocol";
constexpr std::string_view kSslCaLocationField = "ssl_ca_location";

constexpr std::string_view kGroupIdField = "group_id";
constexpr std::string_view kAutoCommitField = "enable_auto_commit";
constexpr std::string_view kAutoOffsetResetField = "auto_offset_reset";

constexpr std::string_view kPodNameSubstr = "{pod_name}";
constexpr std::string_view kEnvPodName = "env_pod_name";

logging::Level CppkafkalogLevelToLoggingLevel(
    const cppkafka::LogLevel& syslog_level) noexcept {
  return impl::convertRdKafkaLogLevelToLoggingLevel(
      static_cast<int>(syslog_level));
}

void CheckComponentNameStart(const std::string& component_name,
                             const std::string& start) {
  // producer's component should start with kafka-producer, consumer's - with
  // kafka-consumer
  if (component_name.rfind(start) != 0) {
    throw std::runtime_error{
        fmt::format("{} doesn't start with {}", component_name, start)};
  }
}

std::optional<std::string> GetPodName(
    const components::ComponentConfig& config) {
  if (!config.HasMember(kEnvPodName)) {
    throw std::runtime_error(
        fmt::format("Found '{}' in group id but not found '{}' in config",
                    kPodNameSubstr, kEnvPodName));
  }
  const auto env_pod_name = config[kEnvPodName].As<std::string>();
  const auto env_variables =
      engine::subprocess::GetCurrentEnvironmentVariablesPtr();
  if (auto pod_name = env_variables->GetValueOptional(env_pod_name)) {
    return *pod_name;
  }
  LOG_WARNING() << fmt::format("Not found '{}' in env variables", env_pod_name);
  return std::nullopt;
}

std::string GetGroupId(const components::ComponentConfig& config) {
  auto group_id = config[kGroupIdField].As<std::string>();
  if (auto pos = group_id.find(kPodNameSubstr); pos != std::string::npos) {
    if (auto pod_name = GetPodName(config)) {
      return group_id.replace(pos, kPodNameSubstr.size(), pod_name.value());
    }
  }
  return group_id;
}

}  // namespace

Secret Parse(const formats::json::Value& doc, formats::parse::To<Secret>) {
  Secret secret;
  secret.username = doc["username"].As<std::string>();
  secret.password = doc["password"].As<std::string>();
  secret.brokers = doc["brokers"].As<std::string>();
  return secret;
}

BrokerSecrets::BrokerSecrets(const formats::json::Value& doc) {
  if (!doc.HasMember("kafka_settings")) {
    LOG_ERROR() << "No kafka_settings in secdist";
  }
  secrets_ = doc["kafka_settings"].As<std::map<std::string, Secret>>();
}

const Secret& BrokerSecrets::GetSecretByComponentName(
    const std::string& component_name) const {
  if (const auto& secret_it = secrets_.find(component_name);
      secret_it != secrets_.end()) {
    return secret_it->second;
  }
  throw std::runtime_error(
      fmt::format("No secrets for {} component", component_name));
}

std::unique_ptr<cppkafka::Configuration> MakeConfiguration(
    const BrokerSecrets& secrets, const components::ComponentConfig& config) {
  auto cppkafka_config = std::make_unique<cppkafka::Configuration>();
  const auto& secret = secrets.GetSecretByComponentName(config.Name());
  cppkafka_config->set("metadata.broker.list", secret.brokers);
  if (config[kSecurityProtocolBrokersField].As<std::string>() == "SASL_SSL") {
    cppkafka_config->set(
        {{"sasl.username", secret.username},
         {"sasl.password", secret.password},
         {"ssl.ca.location", config[kSslCaLocationField].As<std::string>()},
         {"sasl.mechanisms",
          config[kSaslMechanismsBrokersField].As<std::string>()},
         {"security.protocol",
          config[kSecurityProtocolBrokersField].As<std::string>()}});
  }

  cppkafka_config->set_log_callback([](cppkafka::KafkaHandleBase&, int level,
                                       const std::string& facility,
                                       const std::string& message) {
    LOG(CppkafkalogLevelToLoggingLevel(static_cast<cppkafka::LogLevel>(level)))
        << facility << ' ' << message;
  });

  return cppkafka_config;
}

std::unique_ptr<cppkafka::Configuration> SetErrorCallback(
    std::unique_ptr<cppkafka::Configuration> config, impl::Stats& stats) {
  config->set_error_callback([&stats](cppkafka::KafkaHandleBase&,
                                      int error_code,
                                      const std::string& reason) {
    LOG_ERROR() << "error callback: " << reason << ", with code: " << error_code
                << ", description: "
                << cppkafka::Error(static_cast<rd_kafka_resp_err_t>(error_code))
                       .to_string();
    if (error_code == RD_KAFKA_RESP_ERR__RESOLVE ||
        error_code == RD_KAFKA_RESP_ERR__TRANSPORT ||
        error_code == RD_KAFKA_RESP_ERR__AUTHENTICATION ||
        error_code == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
      ++stats.connections_error;
    }
  });

  return config;
}

std::unique_ptr<cppkafka::Configuration> MakeConsumerConfiguration(
    const BrokerSecrets& secrets, const components::ComponentConfig& config) {
  CheckComponentNameStart(config.Name(), "kafka-consumer");
  auto cppkafka_config = MakeConfiguration(secrets, config);
  auto group_id = GetGroupId(config);
  LOG_DEBUG() << fmt::format("Group id for kafka consumer '{}': '{}'",
                             config.Name(), group_id);
  cppkafka_config->set("group.id", std::move(group_id));
  cppkafka_config->set("enable.auto.commit",
                       config[kAutoCommitField].As<bool>());
  cppkafka_config->set("auto.offset.reset",
                       config[kAutoOffsetResetField].As<std::string>());
  cppkafka_config->set("topic.metadata.refresh.interval.ms", "1000");
  cppkafka_config->set_offset_commit_callback(
      [](cppkafka::Consumer&, cppkafka::Error error,
         const cppkafka::TopicPartitionList& topic_partitions) {
        if (error) {
          LOG_WARNING() << fmt::format(
              "Error occured when committing offsets: {}", error.to_string());
        } else {
          for (const auto& topic_partition : topic_partitions) {
            if (topic_partition.get_offset() ==
                topic_partition.OFFSET_INVALID) {
              continue;
            }

            LOG_INFO() << fmt::format(
                "Offset {} committed for topic '{}' in partition {}",
                topic_partition.get_offset(), topic_partition.get_topic(),
                topic_partition.get_partition());
          }
        }
      });

  return cppkafka_config;
}

}  // namespace kafka

USERVER_NAMESPACE_END
