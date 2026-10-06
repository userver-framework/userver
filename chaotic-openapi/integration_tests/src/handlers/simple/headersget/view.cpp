#include <handlers/simple/headersget/handler.hpp>

namespace handlers::simple::headersget {

Response View::Handle(Request&& /*request*/, Deps&& /*deps*/, RequestContext&) {
    Response200 response;
    response.body = response.X_String;
    return response;
}

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::headersget
