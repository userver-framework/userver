#include <userver/chaotic/openapi/middlewares/component_list.hpp>

#include <userver/chaotic/openapi/middlewares/follow_redirects_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/logging_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/proxy_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/ssl_middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::middlewares {

void AppendDefaultMiddlewares(components::ComponentList& component_list) {
    component_list.Append<chaotic::openapi::LoggingMiddlewareFactory>()
        .Append<chaotic::openapi::SslMiddlewareFactory>()
        .Append<chaotic::openapi::ProxyMiddlewareFactory>()
        .Append<chaotic::openapi::FollowRedirectsMiddlewareFactory>();
}

}  // namespace chaotic::openapi::middlewares

USERVER_NAMESPACE_END
