#pragma once

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Request;
class Response;
}  // namespace clients::http

namespace chaotic::openapi::client {

class Middleware {
public:
    virtual void OnRequest(clients::http::Request& request) = 0;

    virtual void OnResponse(clients::http::Response& response) = 0;

    virtual ~Middleware();
};

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
