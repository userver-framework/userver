#pragma once

#include <handlers/simple/sharedtwo/requests.hpp>
#include <handlers/simple/sharedtwo/responses.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/request/request_context.hpp>

namespace handlers::simple::sharedtwo {

struct HandlerTag;

class View final {
public:
    using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;
    static Response Handle(Request&&, Deps&&, USERVER_NAMESPACE::server::request::RequestContext&);
};

}  // namespace handlers::simple::sharedtwo
