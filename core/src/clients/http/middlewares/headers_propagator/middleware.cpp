#include <clients/http/middlewares/headers_propagator/middleware.hpp>

#include <userver/http/url.hpp>
#include <userver/server/request/task_inherited_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::headers_propagator {

void Middleware::HookCreateSpan(MiddlewareRequest& request, tracing::Span&) {
    for (const auto& [name, value] : server::request::GetPropagatedHeaders()) {
        request.SetHeader(name, value);
    }
}

}  // namespace clients::http::middlewares::headers_propagator

USERVER_NAMESPACE_END
