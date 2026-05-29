#include <userver/ugrpc/server/impl/ratelimit_metadata.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void AddRatelimitMetadata(grpc::ServerContextBase& server_context) {
    server_context.AddTrailingMetadata(ugrpc::impl::kXYaTaxiRatelimitedBy, ugrpc::impl::kHostname);
    server_context
        .AddTrailingMetadata(ugrpc::impl::kXYaTaxiRatelimitReason, ugrpc::impl::kCongestionControlRatelimitReason);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
