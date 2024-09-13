#include <userver/clients/http/plugins/headers_propagator/component.hpp>

#include <clients/http/plugins/headers_propagator/plugin.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::headers_propagator {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : ComponentBase(config, context),
      plugin_(std::make_unique<headers_propagator::Plugin>()) {}

Component::~Component() = default;

http::Plugin& Component::GetPlugin() { return *plugin_; }

}  // namespace clients::http::plugins::headers_propagator

USERVER_NAMESPACE_END
