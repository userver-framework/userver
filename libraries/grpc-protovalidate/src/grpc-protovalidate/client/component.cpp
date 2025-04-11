#include <userver/grpc-protovalidate/client/component.hpp>

#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <grpc-protovalidate/client/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

ValidationSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<ValidationSettings>) {
    ValidationSettings settings;
    settings.fail_fast = config["fail-fast"].As<bool>(settings.fail_fast);
    return settings;
}

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.global.fail_fast = config["fail-fast"].As<bool>(settings.global.fail_fast);
    settings.per_method = config["methods"].As<utils::impl::TransparentMap<std::string, ValidationSettings>>({});
    return settings;
}

ValidatorComponent::ValidatorComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ugrpc::client::MiddlewareFactoryComponentBase(
          config,
          context,
          middlewares::MiddlewareDependencyBuilder().InGroup<middlewares::groups::Core>()
      ) {}

std::shared_ptr<const ugrpc::client::MiddlewareBase> ValidatorComponent::CreateMiddleware(
    const ugrpc::client::ClientInfo& /*client_info*/,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema ValidatorComponent::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema ValidatorComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::client::MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC response validator component
additionalProperties: false
properties:
    fail-fast:
        type: boolean
        description: do not check remaining constraints after first error is encountered
        defaultDescription: true
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
)");
}

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
