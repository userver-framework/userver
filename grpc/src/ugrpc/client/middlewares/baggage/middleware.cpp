#include "middleware.hpp"

#include <userver/baggage/baggage_manager.hpp>
#include <userver/logging/log.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/utils/log.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::baggage {

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    const auto* bg = USERVER_NAMESPACE::baggage::BaggageManager::TryGetBaggage();
    if (bg) {
        LOG_DEBUG() << "Send baggage " << bg->ToString();
        auto& client_context = context.GetContext();
        client_context.AddMetadata(ugrpc::impl::kXBaggage, ugrpc::impl::ToGrpcString(bg->ToString()));
    }
}

std::shared_ptr<const MiddlewareBase> MiddlewareFactory::GetMiddleware(std::string_view /*client_name*/) const {
    return std::make_shared<Middleware>();
}

}  // namespace ugrpc::client::middlewares::baggage

USERVER_NAMESPACE_END
