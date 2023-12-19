#include "dynamic_config_defaults.hpp"

#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

DynamicConfigDefaults::DynamicConfigDefaults(
    const components::ComponentContext& component_context)
    : defaults_(component_context.FindComponent<components::DynamicConfig>()
                    .GetDefaultDocsMap()) {}

formats::json::Value DynamicConfigDefaults::Perform(
    const formats::json::Value& /*request_body*/) const {
  return defaults_.AsJson();
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
