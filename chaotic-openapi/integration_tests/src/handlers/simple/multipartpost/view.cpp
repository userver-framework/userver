#include <handlers/simple/multipartpost/handler.hpp>

namespace handlers::simple::multipartpost {

Response View::Handle(Request&& /*request*/, Deps&& /*deps*/) { return {}; }

std::string View::GetRequestBodyForLogging(const std::string& /*body*/) { return {}; }

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    USERVER_NAMESPACE::server::request::RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::multipartpost
