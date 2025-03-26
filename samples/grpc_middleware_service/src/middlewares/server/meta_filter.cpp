#include "meta_filter.hpp"

#include <userver/utest/using_namespace_userver.hpp>

namespace sample::grpc::auth::server {

namespace {

::grpc::string_ref ToRef(const std::string& header) { return ::grpc::string_ref{header.data(), header.size()}; }

}  // namespace

MetaFilter::MetaFilter(std::vector<std::string> headers) : headers_(std::move(headers)) {}

void MetaFilter::Handle(ugrpc::server::MiddlewareCallContext& context) const {
    const auto& metadata = context.GetCall().GetContext().client_metadata();

    for (const auto& header : headers_) {
        const auto it = metadata.find(ToRef(header));
        if (it == metadata.cend()) {
            auto& rpc = context.GetCall();
            rpc.FinishWithError(::grpc::Status{::grpc::StatusCode::FAILED_PRECONDITION, "Missed some headers"});
            LOG_ERROR() << "Missed some headers";
            return;
        }
    }

    context.Next();
}

/// [gRPC middleware sample]
MetaFilterComponent::MetaFilterComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ugrpc::server::MiddlewareFactoryComponentBase(config, context) {}

std::shared_ptr<ugrpc::server::MiddlewareBase> MetaFilterComponent::CreateMiddleware(
    const ugrpc::server::ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<MetaFilter>(middleware_config["headers"].As<std::vector<std::string>>());
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

}  // namespace sample::grpc::auth::server
