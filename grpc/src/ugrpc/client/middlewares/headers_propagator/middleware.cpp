#include <ugrpc/client/middlewares/headers_propagator/middleware.hpp>

#include <userver/server/request/task_inherited_request.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::headers_propagator {

Middleware::Middleware(Settings&& settings) : settings_(std::move(settings)) {}

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    auto& client_context = context.GetClientContext();

    const auto& skip_headers = settings_.skip_headers;

    for (const auto& header : server::request::GetPropagatedHeaders()) {
        auto h = utils::text::ToLower(header.name);
        if (skip_headers.find(h) != skip_headers.end()) {
            continue;
        }
        client_context.AddMetadata(std::move(h), std::string{header.value});
    }
}

}  // namespace ugrpc::client::middlewares::headers_propagator

USERVER_NAMESPACE_END
