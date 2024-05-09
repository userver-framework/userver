#include <kafka/impl/opaque.hpp>

#include <cstdint>

#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

logging::LogExtra CreateLogTags(const std::string& component_name,
                                EntityType entity_type) {
  switch (entity_type) {
    case EntityType::kConsumer:
      return logging::LogExtra{{Opaque::kConsumerLogTagKey, component_name}};
    case EntityType::kProducer:
      return logging::LogExtra{{Opaque::kProducerLogTagKey, component_name}};
  }
}

}  // namespace

Opaque::Opaque(const std::string& component_name, EntityType entity_type)
    : component_name_(component_name),
      log_tags_(CreateLogTags(component_name, entity_type)),
      stats_() {}

Opaque::~Opaque() = default;

Opaque& Opaque::FromPtr(void* opaque_ptr) {
  UINVARIANT(
      reinterpret_cast<std::uintptr_t>(opaque_ptr) % alignof(Opaque) == 0,
      "Incorrect opaque alignment");

  return *static_cast<Opaque*>(opaque_ptr);
}

const std::string& Opaque::GetComponentName() const { return component_name_; }

Stats& Opaque::GetStats() const { return stats_; }

logging::LogExtra Opaque::MakeCallbackLogTags(
    const std::string& callback_name) const {
  auto callback_log_tags{log_tags_};
  callback_log_tags.Extend("kafka_callback", callback_name);

  return callback_log_tags;
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
