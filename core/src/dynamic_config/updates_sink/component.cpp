#include <userver/dynamic_config/updates_sink/component.hpp>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

struct DynamicConfigUpdatesSinkBase::UsedByInfo {
  concurrent::Variable<std::string> component_name_;
};

DynamicConfigUpdatesSinkBase::DynamicConfigUpdatesSinkBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      used_by_(std::make_unique<UsedByInfo>()) {}

DynamicConfigUpdatesSinkBase::~DynamicConfigUpdatesSinkBase() = default;

void DynamicConfigUpdatesSinkBase::SetConfig(
    std::string_view updater, const dynamic_config::DocsMap& config) {
  dynamic_config::DocsMap config_copy = config;
  SetConfig(updater, std::move(config_copy));
}

}  // namespace components

namespace dynamic_config::impl {

void RegisterUpdater(components::DynamicConfigUpdatesSinkBase& sink,
                     std::string_view sink_component_name,
                     std::string_view updater_component_name) {
  auto locked_ptr = sink.used_by_->component_name_.Lock();
  auto& component_name = *locked_ptr;

  if (component_name.empty()) {
    component_name = updater_component_name;
  } else if (sink_component_name == components::DynamicConfig::kName &&
             component_name == components::DynamicConfig::kName) {
    throw std::runtime_error(
        fmt::format("Please set dynamic-config.updates-enabled: true to signal "
                    "that there is a dynamic config updater component"));
  } else {
    throw std::runtime_error(fmt::format(
        "updates sink '{}' can't be used by component '{}' "
        "because it is already used by component '{}'",
        sink_component_name, updater_component_name, component_name));
  }
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
