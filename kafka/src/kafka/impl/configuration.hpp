#pragma once

#include <functional>
#include <memory>

#include <userver/components/component_fwd.hpp>

#include <kafka/impl/entity_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class Opaque;

/// @brief Wrapper for `librdkafka` `rd_kafka_conf_t`. Used as proxy between
/// userver YAML configs and `librdkafka` configuration classes.
/// @see impl/configuration.cpp
/// @see components/consumer_component.cpp
/// @see components/producer_component.cpp
/// @see https://github.com/confluentinc/librdkafka/blob/master/CONFIGURATION.md
class Configuration final {
  using CallbacksSetter = std::function<void(void*)>;

 public:
  /// @brief Sets common Kafka objects configuration options, including broker
  /// SASL authentication mechanism, and consumer/producer specific options.
  Configuration(const components::ComponentConfig& config,
                const components::ComponentContext& context,
                EntityType entity_type);

  ~Configuration();

  Configuration(const Configuration&) = delete;
  Configuration& operator=(const Configuration&) = delete;

  Configuration(Configuration&&) noexcept = default;
  Configuration& operator=(Configuration&&) = delete;

  const std::string& GetComponentName() const;

  /// @brief Set ups the opaque pointer that passed to all `librdkafka`
  /// callbacks.
  Configuration& SetOpaque(Opaque& opaque);

  /// @brief Invokes the @param callback_setter with the stored
  /// `rd_kafka_conf_t` pointer as a parameter. Can be used to set up
  /// consumer/producer specific callbacks.
  Configuration& SetCallbacks(CallbacksSetter callback_setter);

  /// @brief Releases stored `rd_kafka_conf_t` pointer to be passed as a
  /// parameter of `rd_kafka_new` function that takes ownership on
  /// configutation.
  void* Release();

 private:
  void SetCommonOptions(const components::ComponentConfig& config,
                        const components::ComponentContext& context);

  void SetConsumerOptions(const components::ComponentConfig& config);

  void SetProducerOptions(const components::ComponentConfig& config);

 private:
  const std::string component_name_;

  class ConfHolder;
  std::unique_ptr<ConfHolder> conf_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
