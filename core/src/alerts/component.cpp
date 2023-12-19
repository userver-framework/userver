#include <userver/alerts/component.hpp>

#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

StorageComponent::StorageComponent(const components::ComponentConfig&,
                                   const components::ComponentContext&) {}

Storage& StorageComponent::GetStorage() const { return storage_; }

}  // namespace alerts

USERVER_NAMESPACE_END
