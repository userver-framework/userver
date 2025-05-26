#include <userver/ugrpc/client/middlewares/headers_propagator/middleware.hpp>

#include <userver/server/request/task_inherited_request.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::headers_propagator {

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    auto& client_context = context.GetContext();

    for (const auto& header : server::request::GetPropagatedHeaders()) {
        client_context.AddMetadata(utils::text::ToLower(header.name), std::string{header.value});
    }
}

}  // namespace ugrpc::client::middlewares::headers_propagator

USERVER_NAMESPACE_END
