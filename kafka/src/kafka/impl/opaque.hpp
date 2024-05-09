#pragma once

#include <userver/logging/log_extra.hpp>

#include <kafka/impl/entity_type.hpp>
#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

/// @brief Wrapper for `librdkafka` callbacks `void* opaque` param.
/// Should be passed to `Configuration::SetOpaque`.
class Opaque final {
 public:
  static constexpr auto kConsumerLogTagKey{"kafka_consumer"};
  static constexpr auto kProducerLogTagKey{"kafka_producer"};

  Opaque(const std::string& component_name, EntityType entity_type);

  virtual ~Opaque();

  static Opaque& FromPtr(void* opaque_ptr);

  const std::string& GetComponentName() const;
  Stats& GetStats() const;
  logging::LogExtra MakeCallbackLogTags(const std::string& callback_name) const;

 private:
  const std::string component_name_;
  const logging::LogExtra log_tags_;

  mutable Stats stats_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
