#include <userver/grpc-protovalidate/client/component.hpp>

#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <grpc-protovalidate/client/middleware.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/grpc-protovalidate/client/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

ValidationSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<ValidationSettings>) {
    ValidationSettings settings;
    settings.fail_fast = config["fail-fast"].As<bool>(settings.fail_fast);
    settings.validate_requests = config["validate-requests"].As<bool>(settings.validate_requests);
    return settings;
}

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.global.fail_fast = config["fail-fast"].As<bool>(settings.global.fail_fast);
    settings.global.validate_requests = config["validate-requests"].As<bool>(settings.global.validate_requests);
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
    return yaml_config::MergeSchemasFromResource<
        ugrpc::client::MiddlewareFactoryComponentBase>("src/grpc-protovalidate/client/component.yaml");
}

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
