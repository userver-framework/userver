#include <userver/ugrpc/client/middlewares/origin/component.hpp>

#include <userver/components/component_config.hpp>

#include <ugrpc/client/middlewares/origin/middleware.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/client/middlewares/origin/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::origin {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.user_agent = config["user-agent"].As<std::optional<std::string>>();
    return settings;
}

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(config, context)
{}

Component::~Component() = default;

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ugrpc::client::ClientInfo& /*client_info*/,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        MiddlewareFactoryComponentBase>("src/ugrpc/client/middlewares/origin/component.yaml");
}

}  // namespace ugrpc::client::middlewares::origin

USERVER_NAMESPACE_END
