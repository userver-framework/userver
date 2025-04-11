#include <userver/ugrpc/server/middlewares/baggage/middleware.hpp>

#include <userver/baggage/baggage.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/grpc_string_logging.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <dynamic_config/variables/BAGGAGE_SETTINGS.hpp>
#include <dynamic_config/variables/USERVER_BAGGAGE_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::baggage {

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    const auto& dynamic_config = context.GetInitialDynamicConfig();

    if (dynamic_config[::dynamic_config::USERVER_BAGGAGE_ENABLED]) {
        const auto& baggage_settings = dynamic_config[::dynamic_config::BAGGAGE_SETTINGS];

        const auto& server_context = context.GetServerContext();

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
}

}  // namespace ugrpc::server::middlewares::baggage

USERVER_NAMESPACE_END
