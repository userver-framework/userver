#include <userver/ugrpc/client/middlewares/log/component.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

Settings Parse(const yaml_config::YamlConfig& config,
               formats::parse::To<Settings>) {
  Settings settings;
  settings.max_msg_size =
      config["msg-size-log-limit"].As<std::size_t>(settings.max_msg_size);
  settings.log_level =
      config["log-level"].As<logging::Level>(settings.log_level);
  return settings;
}

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context),
      settings_(config.As<Settings>()) {}

std::shared_ptr<const MiddlewareFactoryBase> Component::GetMiddlewareFactory() {
  return std::make_shared<MiddlewareFactory>(*settings_);
}

Component::~Component() = default;

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<MiddlewareComponentBase>(R"(
type: object
description: gRPC service logger component
additionalProperties: false
properties:
    log-level:
        type: string
        description: log level of message log
    msg-size-log-limit:
        type: string
        description: max message size to log, the rest will be truncated
)");
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
