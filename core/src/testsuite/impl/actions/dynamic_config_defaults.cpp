#include "dynamic_config_defaults.hpp"

#include <userver/components/component.hpp>
#include <userver/dynamic_config/fallbacks/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

namespace {

const dynamic_config::DocsMap& GetDynamicConfigDefaults(
    const components::ComponentContext& context) {
  if (const auto* const updater =
          context.FindComponentOptional<components::DynamicConfigFallbacks>()) {
    return updater->GetDefaults(utils::InternalTag{});
  }
  if (const auto* const updater = context.FindComponentOptional<
                                  components::DynamicConfigClientUpdater>()) {
    return updater->GetDefaults(utils::InternalTag{});
  }
  static const dynamic_config::DocsMap kNoDefaults;
  return kNoDefaults;
}

}  // namespace

DynamicConfigDefaults::DynamicConfigDefaults(
    const components::ComponentContext& component_context)
    : defaults_(GetDynamicConfigDefaults(component_context)) {}

formats::json::Value DynamicConfigDefaults::Perform(
    const formats::json::Value& /*request_body*/) const {
  return defaults_.AsJson();
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
