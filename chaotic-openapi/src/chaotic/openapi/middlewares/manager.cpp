#include <userver/chaotic/openapi/middlewares/manager.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

void MiddlewareManager::RegisterMiddleware(std::shared_ptr<client::Middleware> middleware) {
    middlewares_.push_back(middleware);
}

void MiddlewareManager::ProcessRequest(clients::http::Request& request) {
    for (const auto& middleware : middlewares_) {
        middleware->OnRequest(request);
    }
}

void MiddlewareManager::ProcessResponse(clients::http::Response& response) {
    for (const auto& middleware : middlewares_) {
        middleware->OnResponse(response);
    }
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
