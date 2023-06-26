#include <userver/ugrpc/client/middlewares/log/component.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context),
      max_size_(config["msg-size-log-limit"].As<std::size_t>(512)) {
  log_level_ = config["log-level"].As<logging::Level>(logging::Level::kDebug);
}

std::shared_ptr<const MiddlewareFactoryBase> Component::GetMiddlewareFactory() {
  return std::make_shared<MiddlewareFactory>(
      Middleware::Settings{max_size_, log_level_});
}

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
