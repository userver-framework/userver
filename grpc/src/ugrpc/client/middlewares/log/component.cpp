#include <userver/ugrpc/client/middlewares/log/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.log_level = config["log-level"].As<logging::Level>(settings.log_level);
    settings.msg_log_level = config["msg-log-level"].As<logging::Level>(settings.msg_log_level);
    settings.max_msg_size = config["msg-size-log-limit"].As<std::size_t>(settings.max_msg_size);
    return settings;
}

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::Logging>()
      ) {}

Component::~Component() = default;

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ugrpc::client::ClientInfo& /*client_info*/,
    const yaml_config::YamlConfig& middleware_config
) const {
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
)");
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
