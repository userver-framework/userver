#include <userver/grpc-protovalidate/server/component.hpp>

#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <grpc-protovalidate/server/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::server {

ValidationSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<ValidationSettings>) {
    ValidationSettings settings;
    settings.fail_fast = config["fail-fast"].As<bool>(settings.fail_fast);
    settings.send_violations = config["send-violations"].As<bool>(settings.send_violations);
    return settings;
}

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.global.fail_fast = config["fail-fast"].As<bool>(settings.global.fail_fast);
    settings.global.send_violations = config["send-violations"].As<bool>(settings.global.send_violations);
    settings.per_method = config["methods"].As<utils::impl::TransparentMap<std::string, ValidationSettings>>({});
    return settings;
}

ValidatorComponent::ValidatorComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ugrpc::server::MiddlewareFactoryComponentBase(
          config,
          context,
          middlewares::MiddlewareDependencyBuilder()
              .InGroup<middlewares::groups::Core>()
              .After<ugrpc::server::middlewares::deadline_propagation::Component>(middlewares::DependencyType::kWeak)
      ) {}

std::shared_ptr<const ugrpc::server::MiddlewareBase> ValidatorComponent::CreateMiddleware(
    const ugrpc::server::ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema ValidatorComponent::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema ValidatorComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::server::MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC request validator component
additionalProperties: false
properties:
    fail-fast:
        type: boolean
        description: do not check remaining constraints after first error is encountered
        defaultDescription: true
    send-violations:
        type: boolean
        description: send found constraint violations in grpc::Status
        defaultDescription: false
    methods:
        type: object
        description: per-method middleware options overrides
        properties: {}
        additionalProperties:
            type: object
            description: method options
            additionalProperties: false
            properties:
                fail-fast:
                    type: boolean
                    description: see 'fail-fast' global option
                send-violations:
                    type: boolean
                    description: see 'send-violations' global option
)");
}

}  // namespace grpc_protovalidate::server

USERVER_NAMESPACE_END
