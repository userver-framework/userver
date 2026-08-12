#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/middlewares/groups.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/server/middlewares/deadline_propagation/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    return Settings{
        .prefer_absolute_deadline = config["prefer-absolute-deadline"].As<bool>(false),
    };
}

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::Core>()
              .After<congestion_control::Component>(USERVER_NAMESPACE::middlewares::DependencyType::kWeak)
      )
{}

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ugrpc::server::ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        MiddlewareFactoryComponentBase>("src/ugrpc/server/middlewares/deadline_propagation/component.yaml");
}

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
