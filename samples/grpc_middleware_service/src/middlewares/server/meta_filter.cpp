#include "meta_filter.hpp"

#include <userver/ugrpc/server/middlewares/headers_propagator/component.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace samples::grpc::auth::server {

namespace {

::grpc::string_ref ToRef(const std::string& header) { return ::grpc::string_ref{header.data(), header.size()}; }

}  // namespace

MetaFilter::MetaFilter(MiddlewareConfig&& config) : headers_(std::move(config.headers)) {}

void MetaFilter::OnCallStart(ugrpc::server::MiddlewareCallContext& context) const {
    const auto& metadata = context.GetServerContext().client_metadata();

    for (const auto& header : headers_) {
        const auto it = metadata.find(ToRef(header));
        if (it == metadata.cend()) {
            LOG_ERROR() << "Missed some headers";
            return context.SetError(::grpc::Status{::grpc::StatusCode::FAILED_PRECONDITION, "Missed some headers"});
        }
    }
}

/// [gRPC middleware sample]

MiddlewareConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareConfig>) {
    MiddlewareConfig config;
    config.headers = value["headers"].As<std::vector<std::string>>();
    return config;
}

/// [MiddlewareDependencyBuilder After]
MetaFilterComponent::MetaFilterComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ugrpc::server::MiddlewareFactoryComponentBase(
          config,
          context,
          middlewares::MiddlewareDependencyBuilder()
              .InGroup<middlewares::groups::User>()
              .After<ugrpc::server::middlewares::headers_propagator::Component>()
      ) {}
/// [MiddlewareDependencyBuilder After]

std::shared_ptr<const ugrpc::server::MiddlewareBase> MetaFilterComponent::CreateMiddleware(
    const ugrpc::server::ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<MetaFilter>(middleware_config.As<MiddlewareConfig>());
}

yaml_config::Schema MetaFilterComponent::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema MetaFilterComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC service logger component
additionalProperties: false
properties:
    headers:
        type: array
        description: headers names to filter
        items:
            type: string
            description: header name
)");
}
/// [gRPC middleware sample]

}  // namespace samples::grpc::auth::server
