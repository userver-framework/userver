#pragma once

#include <string>

#include <handlers/simple/strongget/requests.hpp>
#include <handlers/simple/strongget/responses.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/request/request_context.hpp>

namespace handlers::simple::strongget {

struct HandlerTag;

class View final {
public:
    using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;
    using RequestContext = USERVER_NAMESPACE::server::request::RequestContext;

    static Response Handle(Request&& request, Deps&& deps, RequestContext& context);

    static std::string GetResponseForLogging(
        const Response& response,
        const std::string& serialized_response,
        RequestContext& context
    );
};

}  // namespace handlers::simple::strongget
