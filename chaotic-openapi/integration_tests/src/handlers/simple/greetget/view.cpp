#include <handlers/simple/greetget/handler.hpp>

namespace handlers::simple::greetget {

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

}  // namespace handlers::simple::greetget
