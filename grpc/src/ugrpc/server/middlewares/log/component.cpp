#include <userver/ugrpc/server/middlewares/log/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/ugrpc/server/middlewares/access_log/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.log_level = config["log-level"].As<logging::Level>(settings.log_level);
    settings.msg_log_level = config["msg-log-level"].As<logging::Level>(settings.msg_log_level);
    settings.max_msg_size = config["msg-size-log-limit"].As<std::size_t>(settings.max_msg_size);
    settings.local_log_level = config["local-log-level"].As<logging::Level>(settings.local_log_level);
    return settings;
}

/// [middleware InGroup]
Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::Logging>()
              .After<access_log::Component>(USERVER_NAMESPACE::middlewares::DependencyType::kWeak)
      ) {}
/// [middleware InGroup]

std::shared_ptr<const MiddlewareBase>
Component::CreateMiddleware(const ugrpc::server::ServiceInfo&, const yaml_config::YamlConfig& middleware_config) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC service logger component
additionalProperties: false
properties:
    log-level:
        type: string
        description: set log level threshold
    msg-log-level:
        type: string
        description: set up logging level for request/response messages
    msg-size-log-limit:
        type: string
        description: max message size to log, the rest will be truncated
    local-log-level:
        type: string
        description: local log level of the span for user-provided handler
)");
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
