#include "handler.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/http/common_headers.hpp>

namespace userver_httparena::plaintext {
const std::string kContentTypeTextPlain{"text/plain"};

std::string Handler::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    request.GetHttpResponse().SetHeader(http::headers::kContentType, kContentTypeTextPlain);
    return GetResponse();
}

std::string Handler::GetResponse() { return "ok"; }
}  // namespace userver_httparena::plaintext
