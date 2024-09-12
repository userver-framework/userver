#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include <librdkafka/rdkafka.h>

#include <userver/formats/parse/to.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/holders.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct Secret;

struct CommonConfiguration final {
  std::chrono::milliseconds topic_metadata_refresh_interval{300000};
  std::chrono::milliseconds metadata_max_age{900000};
};

struct SecurityConfiguration final {
  struct Plaintext {};
  struct SaslSsl {
    std::string security_mechanism;
    std::string ssl_ca_location;
  };

  using SecurityProtocol = std::variant<Plaintext, SaslSsl>;
  SecurityProtocol security_protocol{};
};

/// Arbitrary options for `librdkafka` library
/// @see https://github.com/confluentinc/librdkafka/blob/master/CONFIGURATION.md
using RdKafkaOptions = std::unordered_map<std::string, std::string>;

struct ConsumerConfiguration final {
  CommonConfiguration common{};
  SecurityConfiguration security{};

  std::string group_id;
  std::string auto_offset_reset{"smallest"};
  std::optional<std::string> env_pod_name{};
  std::chrono::milliseconds max_callback_duration{300000};

  RdKafkaOptions rd_kafka_options;
};

struct ProducerConfiguration final {
  CommonConfiguration common{};
  SecurityConfiguration security{};

  std::chrono::milliseconds delivery_timeout{3000};
  std::chrono::milliseconds queue_buffering_max{10};
  bool enable_idempotence{false};
  std::uint32_t queue_buffering_max_messages{100000};
  std::uint32_t queue_buffering_max_kbytes{1048576};  // 1 GiB
  std::uint32_t message_max_bytes{1000000};           // ~ 1 MiB
  std::uint32_t message_send_max_retries{2147483647};
  std::chrono::milliseconds retry_backoff{100};
  std::chrono::milliseconds retry_backoff_max{1000};

  RdKafkaOptions rd_kafka_options;
};

CommonConfiguration Parse(const yaml_config::YamlConfig& config,
                          formats::parse::To<CommonConfiguration>);
SecurityConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<SecurityConfiguration>);
ConsumerConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<ConsumerConfiguration>);
ProducerConfiguration Parse(const yaml_config::YamlConfig& config,
                            formats::parse::To<ProducerConfiguration>);

/// @brief Wrapper for `librdkafka` `rd_kafka_conf_t`. Used as proxy between
/// userver YAML configs and `librdkafka` configuration classes.
/// @see impl/configuration.cpp
/// @see components/consumer_component.cpp
/// @see components/producer_component.cpp
/// @see https://github.com/confluentinc/librdkafka/blob/master/CONFIGURATION.md
class Configuration final {
 public:
  /// @brief Sets common Kafka objects configuration options, including broker
  /// SASL authentication mechanism, and consumer/producer specific options.
  Configuration(const std::string& name,
                const ConsumerConfiguration& configuration,
                const Secret& secrets);

  Configuration(const std::string& name,
                const ProducerConfiguration& configuration,
                const Secret& secrets);

  ~Configuration() = default;

  Configuration(const Configuration&) = delete;
  Configuration& operator=(const Configuration&) = delete;

  Configuration(Configuration&&) noexcept = default;
  Configuration& operator=(Configuration&&) = delete;

  const std::string& GetName() const;

  std::string GetOption(const char* option) const;

  /// @brief Releases stored conf to be passed as a
  /// parameter of `rd_kafka_new` function that takes ownership on
  /// configuration.
  ConfHolder Release() &&;

 private:
  void SetCommon(const CommonConfiguration& common);

  void SetSecurity(const SecurityConfiguration& security,
                   const Secret& secrets);

  void SetRdKafka(const RdKafkaOptions& rd_kafka_options);

  void SetConsumer(const ConsumerConfiguration& config);
  void SetProducer(const ProducerConfiguration& config);

  template <class T>
  void SetOption(const char* option, const char* value, T to_print);

  void SetOption(const char* option, const std::string& value);
  void SetOption(const char* option, std::chrono::milliseconds value);
  void SetOption(const char* option, std::uint32_t value);
  void SetOption(const char* option, const Secret::SecretType& value);

 private:
  std::string name_;

  ConfHolder conf_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
