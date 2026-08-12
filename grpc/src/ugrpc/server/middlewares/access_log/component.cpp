#include <userver/ugrpc/server/middlewares/access_log/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/access_log/middleware.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/server/middlewares/access_log/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::access_log {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::Logging>()
      ),
      logging_component_(context.FindComponent<components::Logging>())
{}

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    Settings settings;
    const auto logger_name = middleware_config["access-tskv-logger"].As<std::string>();
    settings.access_tskv_logger = logging_component_.GetTextLogger(logger_name);
    return std::make_shared<Middleware>(std::move(settings));
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        MiddlewareFactoryComponentBase>("src/ugrpc/server/middlewares/access_log/component.yaml");
}

}  // namespace ugrpc::server::middlewares::access_log

USERVER_NAMESPACE_END
