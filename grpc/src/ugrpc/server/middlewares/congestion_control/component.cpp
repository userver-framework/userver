#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/congestion_control/middleware.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    auto reject_status_code = config["grpc-status-code"].As<std::optional<std::string>>();
    if (reject_status_code) {
        settings.reject_status_code = ugrpc::StatusCodeFromString(*reject_status_code);
        if (settings.reject_status_code == grpc::StatusCode::OK) {
            throw std::invalid_argument("grpc-status-code must not be OK status");
        }
    }
    return settings;
}

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::Core>()
      ) {
    auto* cc_component = context.FindComponentOptional<USERVER_NAMESPACE::congestion_control::Component>();

    auto& server = context.FindComponent<ServerComponent>().GetServer();

    if (cc_component) {
        auto& server_limiter = cc_component->GetServerLimiter();
        server_limiter.RegisterLimitee(*this);

        auto& server_sensor = cc_component->GetServerSensor();
        server_sensor.RegisterRequestsSource(server);
    }
}

std::shared_ptr<const MiddlewareBase>
Component::CreateMiddleware(const ugrpc::server::ServiceInfo&, const yaml_config::YamlConfig& middleware_config) const {
    auto middleware = std::make_shared<Middleware>(middleware_config.As<Settings>(), rate_limit_);
    return middleware;
}

void Component::SetLimit(std::optional<size_t> new_limit) {
    if (new_limit) {
        const auto rps_val = *new_limit;
        if (rps_val > 0) {
            rate_limit_->SetMaxSize(rps_val);
            rate_limit_->SetRefillPolicy({1, utils::TokenBucket::Duration{std::chrono::seconds(1)} / rps_val});
        } else {
            rate_limit_->SetMaxSize(0);
        }
    } else {
        rate_limit_->SetMaxSize(1);  // in case it was zero
        rate_limit_->SetInstantRefillPolicy();
    }
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC congestion control component
additionalProperties: false
properties:
    grpc-status-code:
        type: string
        description: gRPC status code for ratelimited responses
        defaultDescription: RESOURCE_EXHAUSTED
)");
}

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
