#include <userver/chaotic/openapi/server/parameters_write.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::server {

ParameterSinkHttpResponse::ParameterSinkHttpResponse(USERVER_NAMESPACE::server::http::HttpResponse& response)
    : response_(response)
{}

void ParameterSinkHttpResponse::SetHeader(std::string_view name, std::string&& value) {
    response_.SetHeader(name, std::move(value));
}

void ParameterSinkHttpResponse::SetCookie(std::string_view /*name*/, std::string&& /*value*/) {
    UINVARIANT(false, "SetCookie called on response parameter sink; response parameters may only be headers");
}

void ParameterSinkHttpResponse::SetPath(chaotic::openapi::Name& /*name*/, std::string&& /*value*/) {
    UINVARIANT(false, "SetPath called on response parameter sink; response parameters may only be headers");
}

void ParameterSinkHttpResponse::SetQuery(std::string_view /*name*/, std::string&& /*value*/) {
    UINVARIANT(false, "SetQuery called on response parameter sink; response parameters may only be headers");
}

void ParameterSinkHttpResponse::SetMultiQuery(std::string_view /*name*/, std::vector<std::string>&& /*value*/) {
    UINVARIANT(false, "SetMultiQuery called on response parameter sink; response parameters may only be headers");
}

}  // namespace chaotic::openapi::server

USERVER_NAMESPACE_END
