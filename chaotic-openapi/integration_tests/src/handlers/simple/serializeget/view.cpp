#include <handlers/simple/serializeget/handler.hpp>

namespace handlers::simple::serializeget {

Response View::Handle(Request&& /*request*/, Deps&& /*deps*/, RequestContext& /*context*/) {
    Response200 response;
    response.body.required_field = "response";
    response.body.optional_field = 42;
    response.X_Serialization_Test = "characterization";
    return response;
}

std::string View::GetResponseForLogging(
    const Response& /*response*/,
    const std::string& /*serialized_response*/,
    RequestContext& /*context*/
) {
    return {};
}

}  // namespace handlers::simple::serializeget
