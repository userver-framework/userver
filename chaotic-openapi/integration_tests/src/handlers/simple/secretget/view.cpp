#include <handlers/simple/secretget/handler.hpp>

namespace handlers::simple::secretget {

Response View::Handle(
    Request&& /*request*/,
    Deps&& /*deps*/,
    RequestContext& /*context*/
) {
    return {};
}

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::secretget
