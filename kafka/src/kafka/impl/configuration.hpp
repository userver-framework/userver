#pragma once

#include <userver/components/component_fwd.hpp>

#include <kafka/impl/entity_type.hpp>

#include <librdkafka/rdkafka.h>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

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
  Configuration(const components::ComponentConfig& config,
                const components::ComponentContext& context,
                EntityType entity_type);

  ~Configuration();

  Configuration(const Configuration&) = delete;
  Configuration& operator=(const Configuration&) = delete;

  Configuration(Configuration&&) noexcept;
  Configuration& operator=(Configuration&&) = delete;

  const std::string& GetComponentName() const;

  /// @brief Releases stored `rd_kafka_conf_t` pointer to be passed as a
  /// parameter of `rd_kafka_new` function that takes ownership on
  /// configutation.
  rd_kafka_conf_t* Release();

 private:
  void SetCommonOptions(const components::ComponentConfig& config,
                        const components::ComponentContext& context);

  void SetConsumerOptions(const components::ComponentConfig& config);

  void SetProducerOptions(const components::ComponentConfig& config);

 private:
  std::string component_name_;

  // Pointer to `conf_` must be released and passed to `rd_kafka_new`
  rd_kafka_conf_t* conf_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
