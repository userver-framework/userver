#pragma once

#include <string>

#include <handlers/simple/sharedput/requests.hpp>
#include <handlers/simple/sharedput/responses.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/request/request_context.hpp>

namespace handlers::simple::sharedput {

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

}  // namespace handlers::simple::sharedput
