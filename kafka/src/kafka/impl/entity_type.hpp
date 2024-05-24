#pragma once

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

enum class EntityType {
  kConsumer,
  kProducer,
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
