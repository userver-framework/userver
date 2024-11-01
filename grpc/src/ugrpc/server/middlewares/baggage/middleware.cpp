#include "middleware.hpp"

#include <userver/baggage/baggage.hpp>
#include <userver/baggage/baggage_settings.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/grpc_string_logging.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::baggage {

void Middleware::Handle(MiddlewareCallContext& context) const {
    auto& call = context.GetCall();

    const auto& dynamic_config = context.GetInitialDynamicConfig();

    if (dynamic_config[USERVER_NAMESPACE::baggage::kBaggageEnabled]) {
        const auto& baggage_settings = dynamic_config[USERVER_NAMESPACE::baggage::kBaggageSettings];

        const auto& server_context = call.GetContext();

        const auto* baggage_header = utils::FindOrNullptr(server_context.client_metadata(), ugrpc::impl::kXBaggage);

        if (baggage_header) {
            LOG_DEBUG() << "Got baggage header: " << *baggage_header;

            auto baggage = USERVER_NAMESPACE::baggage::TryMakeBaggage(
                ugrpc::impl::ToString(*baggage_header), baggage_settings.allowed_keys
            );
            if (baggage) {
                USERVER_NAMESPACE::baggage::kInheritedBaggage.Set(std::move(*baggage));
            }
        }
    }

    context.Next();
}

}  // namespace ugrpc::server::middlewares::baggage

USERVER_NAMESPACE_END
