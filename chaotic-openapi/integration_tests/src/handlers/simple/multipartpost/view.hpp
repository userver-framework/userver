#pragma once

#include <string>

#include <handlers/simple/multipartpost/requests.hpp>
#include <handlers/simple/multipartpost/responses.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/request/request_context.hpp>

namespace handlers::simple::multipartpost {

struct HandlerTag;

class View final {
public:
    using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;

    static Response Handle(Request&& request, Deps&& deps);

    static std::string GetRequestBodyForLogging(const std::string& body);

    static std::string GetResponseForLogging(
        const Response& response,
        const std::string& serialized_response,
        USERVER_NAMESPACE::server::request::RequestContext& context
    );
};

}  // namespace handlers::simple::multipartpost
