#include <handlers/simple/secretget/handler.hpp>

namespace handlers::simple::secretget {

Response View::Handle(Request&& /*request*/, Deps&& /*deps*/) { return {}; }

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    USERVER_NAMESPACE::server::request::RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::secretget
