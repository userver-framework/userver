#include <userver/ugrpc/server/middlewares/log/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/ugrpc/middlewares/pipeline.hpp>
#include <userver/ugrpc/server/middlewares/groups.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.local_log_level = config["log-level"].As<std::optional<logging::Level>>();
    settings.msg_log_level = config["msg-log-level"].As<logging::Level>(settings.msg_log_level);
    settings.max_msg_size = config["msg-size-log-limit"].As<std::size_t>(settings.max_msg_size);
    settings.trim_secrets = config["trim-secrets"].As<bool>(settings.trim_secrets);
    return settings;
}

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          ugrpc::middlewares::MiddlewareDependencyBuilder().InGroup<groups::Logging>()
      ) {}

Component::~Component() = default;

std::shared_ptr<MiddlewareBase>
Component::CreateMiddleware(const ServiceInfo&, const yaml_config::YamlConfig& middleware_config) const {
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
        description: gRPC handlers log level
    msg-log-level:
        type: string
        description: gRPC message body logging level
    msg-size-log-limit:
        type: string
        description: max message size to log, the rest will be truncated
    trim-secrets:
        type: boolean
        description: |
            trim the secrets from logs as marked by the protobuf option.
            you should set this to false if the responses contain
            optional fields and you are using protobuf prior to 3.13
)");
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
