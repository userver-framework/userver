#include <handlers/simple/headersget/handler.hpp>

namespace handlers::simple::headersget {

Response View::Handle(
    Request&& /*request*/,
    Deps&& /*deps*/,
    USERVER_NAMESPACE::server::request::RequestContext& context
) {
    Response200 response;
    const auto* user_id = context.GetDataOptional<std::string>("x-user-id");
    response.X_String = user_id ? *user_id : "";
    response.body = response.X_String;
    return response;
}

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    USERVER_NAMESPACE::server::request::RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::headersget
