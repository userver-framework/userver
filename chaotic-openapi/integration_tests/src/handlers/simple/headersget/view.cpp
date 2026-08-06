#include <handlers/simple/headersget/handler.hpp>

namespace handlers::simple::headersget {

Response View::Handle(Request&& /*request*/, Deps&& /*deps*/) { return {}; }

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    USERVER_NAMESPACE::server::request::RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::headersget
