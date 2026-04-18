#include <ugrpc/server/middlewares/graceful_shutdown_headers/middleware.hpp>

#include <dynamic_config/variables/GRACEFUL_SHUTDOWN_HEADERS.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::graceful_shutdown_headers {

namespace {

struct Identity {};

inline const utils::AnyStorageDataTag<ugrpc::server::StorageContext, Identity> kGracefulShutdownHeadersSet;

using AddMetadataMethod = decltype(&grpc::ServerContext::AddInitialMetadata);

bool SetHeadersIfNeeded(
    const dynamic_config::Snapshot& config,
    MiddlewareCallContext& context,
    AddMetadataMethod add_metadata_method
) {
    const auto& graceful_shutdown_headers = config[::dynamic_config::GRACEFUL_SHUTDOWN_HEADERS];
    if (!graceful_shutdown_headers.enabled) {
        return false;
    }
    auto& server_context = context.GetServerContext();
    for (const auto& [name, values] : graceful_shutdown_headers.headers.extra) {
        auto header_name = ugrpc::impl::ToGrpcString(name);
        for (const auto& value : values) {
            (server_context.*add_metadata_method)(header_name, ugrpc::impl::ToGrpcString(value));
        }
    }
    return true;
}

}  // namespace

Middleware::Middleware(const components::State& state, dynamic_config::Source source)
    : state_(state),
      source_(std::move(source))
{}

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    if (state_.GetServiceLifetimeStage() != components::ServiceLifetimeStage::kGracefulShutdown) {
        return;
    }
    if (SetHeadersIfNeeded(context.GetInitialDynamicConfig(), context, &grpc::ServerContext::AddInitialMetadata)) {
        context.GetStorageContext().Set(kGracefulShutdownHeadersSet, {});
    }
}

void Middleware::PreSendStatus(MiddlewareCallContext& context, grpc::Status&) const {
    if (state_.GetServiceLifetimeStage() != components::ServiceLifetimeStage::kGracefulShutdown) {
        return;
    }
    if (context.GetStorageContext().GetOptional(kGracefulShutdownHeadersSet) != nullptr) {
        return;
    }

    SetHeadersIfNeeded(source_.GetSnapshot(), context, &grpc::ServerContext::AddTrailingMetadata);
}

}  // namespace ugrpc::server::middlewares::graceful_shutdown_headers

USERVER_NAMESPACE_END
