#pragma once

#include <userver/chaotic/openapi/client/middleware.hpp>

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Request;
class Response;
}  // namespace clients::http

namespace chaotic::openapi {

class MiddlewareManager {
public:
    void RegisterMiddleware(std::shared_ptr<client::Middleware> middleware);

    void ProcessRequest(clients::http::Request& request);
    void ProcessResponse(clients::http::Response& response);

private:
    std::vector<std::shared_ptr<client::Middleware>> middlewares_;
};

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
