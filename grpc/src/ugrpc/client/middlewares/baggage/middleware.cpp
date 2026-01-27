#include <userver/ugrpc/client/middlewares/baggage/middleware.hpp>

#include <userver/baggage/baggage_manager.hpp>
#include <userver/logging/log.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::baggage {

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    const auto* bg = USERVER_NAMESPACE::baggage::BaggageManager::TryGetBaggage();
    if (bg) {
        LOG_DEBUG() << "Send baggage " << bg->ToString();
        auto& client_context = context.GetClientContext();
        client_context.AddMetadata(ugrpc::impl::kXBaggage, ugrpc::impl::ToGrpcString(bg->ToString()));
    }
}

}  // namespace ugrpc::client::middlewares::baggage

USERVER_NAMESPACE_END
