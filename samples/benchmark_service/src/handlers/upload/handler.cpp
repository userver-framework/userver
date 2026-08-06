#include "handler.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/http/common_headers.hpp>

namespace userver_httparena::upload {
std::string Handler::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    const auto& body = request.RequestBody();
    request.GetHttpResponse().SetHeader(http::headers::kContentType, "text/plain");
    return std::to_string(body.size());
}
}  // namespace userver_httparena::upload
