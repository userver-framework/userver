#include <taxi_config/config_ptr.hpp>

#include <components/component_context.hpp>
#include <taxi_config/storage/component.hpp>

namespace taxi_config::impl {

const impl::Storage& FindStorage(const components::ComponentContext& context) {
  const auto& component = context.FindComponent<components::TaxiConfig>();
  component.Get();  // wait for the first update to finish
  return *component.cache_;
}

}  // namespace taxi_config::impl
