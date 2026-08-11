#include "handler.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <charconv>

#include <userver/http/common_headers.hpp>

namespace userver_httparena::baseline11 {
const std::string kContentTypeTextPlain{"text/plain"};

std::string Handler::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    const auto& a = request.GetArg("a");
    const auto& b = request.GetArg("b");
    std::string body;
    if (request.GetMethod() == server::http::HttpMethod::kPost) {
        body = request.RequestBody();
    }
    request.GetHttpResponse().SetHeader(http::headers::kContentType, kContentTypeTextPlain);
    return GetResponse(a, b, body);
}

std::string Handler::GetResponse(const std::string& a, const std::string& b, const std::string& body) {
    int sum = 0;
    std::from_chars(a.data(), a.data() + a.size(), sum);
    int b_val = 0;
    std::from_chars(b.data(), b.data() + b.size(), b_val);
    sum += b_val;
    if (!body.empty()) {
        int body_val = 0;
        std::from_chars(body.data(), body.data() + body.size(), body_val);
        sum += body_val;
    }
    return std::to_string(sum);
}
}  // namespace userver_httparena::baseline11
