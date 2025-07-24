#include "chaos.hpp"

#include <middlewares/auth.hpp>

#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/rand.hpp>

namespace samples::grpc::auth::client {

const ::grpc::string kExperinetFeature = "x-my-experiments-enabled";
const ::grpc::string kEnable = "enable";
const ::grpc::string kDisable = "disable";

ChaosMiddleware::ChaosMiddleware(bool feature_enabled) : feature_enabled_(feature_enabled) {}

void ChaosMiddleware::PreStartCall(ugrpc::client::MiddlewareCallContext& context) const {
    const auto rand = utils::Rand();
    const auto enabled = feature_enabled_ && rand % 10 == 0;
    context.GetClientContext().AddMetadata(kExperinetFeature, enabled ? kEnable : kDisable);
}

/// [gRPC middleware sample]
ChaosComponent::ChaosComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ugrpc::client::MiddlewareFactoryComponentBase(config, context) {}

std::shared_ptr<const ugrpc::client::MiddlewareBase> ChaosComponent::CreateMiddleware(
    const ugrpc::client::ClientInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<ChaosMiddleware>(middleware_config["x-my-experiments-enabled"].As<bool>(false));
}

yaml_config::Schema ChaosComponent::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema ChaosComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::client::MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC service logger component
additionalProperties: false
properties:
    x-my-experiments-enabled:
        type: boolean
        description: enable or disable feature
)");
}
/// [gRPC middleware sample]

}  // namespace samples::grpc::auth::client
