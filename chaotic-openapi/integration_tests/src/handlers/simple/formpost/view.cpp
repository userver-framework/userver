#include <handlers/simple/formpost/handler.hpp>

namespace handlers::simple::formpost {

Response View::Handle(
    Request&& /*request*/,
    Deps&& /*deps*/,
    RequestContext& /*context*/
) {
    return {};
}

std::string View::GetRequestBodyForLogging(const std::string& /*body*/) { return {}; }

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::formpost
