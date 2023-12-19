#include <userver/dynamic_config/updates_sink/find.hpp>

#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

components::DynamicConfigUpdatesSinkBase& FindUpdatesSink(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  const auto sink_name =
      config["updates-sink"].As<std::string>(components::DynamicConfig::kName);
  auto& sink = context.FindComponent<components::DynamicConfigUpdatesSinkBase>(
      sink_name);
  impl::RegisterUpdater(sink, sink_name, config.Name());
  return sink;
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
