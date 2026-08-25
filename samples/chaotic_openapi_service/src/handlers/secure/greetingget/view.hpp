#pragma once

#include <string>

#include <handlers/secure/greetingget/requests.hpp>
#include <handlers/secure/greetingget/responses.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/request/request_context.hpp>

namespace handlers::secure::greetingget {

struct HandlerTag;

class View final {
public:
    using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;
    using RequestContext = USERVER_NAMESPACE::server::request::RequestContext;

    static Response Handle(Request&& request, Deps&& deps, RequestContext& context);
};

}  // namespace handlers::secure::greetingget