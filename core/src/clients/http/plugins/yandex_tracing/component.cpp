#include <userver/clients/http/plugins/yandex_tracing/component.hpp>

#include <clients/http/plugins/yandex_tracing/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::yandex_tracing {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : ComponentBase(config, context),
      plugin_(std::make_unique<yandex_tracing::Plugin>()) {}

Component::~Component() = default;

http::Plugin& Component::GetPlugin() { return *plugin_; }

}  // namespace clients::http::plugins::yandex_tracing

USERVER_NAMESPACE_END
