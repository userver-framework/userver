#include <userver/server/http/http_request.hpp>

#include <server/http/http_request_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

HttpRequest::HttpRequest(HttpRequestImpl& impl) : impl_(impl) {}

request::ResponseBase& HttpRequest::GetResponse() const {
  return impl_.GetResponse();
}

HttpResponse& HttpRequest::GetHttpResponse() const {
  return impl_.GetHttpResponse();
}

const HttpMethod& HttpRequest::GetMethod() const { return impl_.GetMethod(); }

const std::string& HttpRequest::GetMethodStr() const {
  return impl_.GetMethodStr();
}

int HttpRequest::GetHttpMajor() const { return impl_.GetHttpMajor(); }

int HttpRequest::GetHttpMinor() const { return impl_.GetHttpMinor(); }

const std::string& HttpRequest::GetUrl() const { return impl_.GetUrl(); }

const std::string& HttpRequest::GetRequestPath() const {
  return impl_.GetRequestPath();
}

const std::string& HttpRequest::GetPathSuffix() const {
  return impl_.GetPathSuffix();
}

std::chrono::duration<double> HttpRequest::GetRequestTime() const {
  return impl_.GetRequestTime();
}

std::chrono::duration<double> HttpRequest::GetResponseTime() const {
  return impl_.GetResponseTime();
}

const std::string& HttpRequest::GetHost() const { return impl_.GetHost(); }

const std::string& HttpRequest::GetArg(const std::string& arg_name) const {
  return impl_.GetArg(arg_name);
}

const std::vector<std::string>& HttpRequest::GetArgVector(
    const std::string& arg_name) const {
  return impl_.GetArgVector(arg_name);
}

bool HttpRequest::HasArg(const std::string& arg_name) const {
  return impl_.HasArg(arg_name);
}

size_t HttpRequest::ArgCount() const { return impl_.ArgCount(); }

std::vector<std::string> HttpRequest::ArgNames() const {
  return impl_.ArgNames();
}

const FormDataArg& HttpRequest::GetFormDataArg(
    const std::string& arg_name) const {
  return impl_.GetFormDataArg(arg_name);
}

const std::vector<FormDataArg>& HttpRequest::GetFormDataArgVector(
    const std::string& arg_name) const {
  return impl_.GetFormDataArgVector(arg_name);
}

bool HttpRequest::HasFormDataArg(const std::string& arg_name) const {
  return impl_.HasFormDataArg(arg_name);
}

size_t HttpRequest::FormDataArgCount() const {
  return impl_.FormDataArgCount();
}

std::vector<std::string> HttpRequest::FormDataArgNames() const {
  return impl_.FormDataArgNames();
}

const std::string& HttpRequest::GetPathArg(const std::string& arg_name) const {
  return impl_.GetPathArg(arg_name);
}

const std::string& HttpRequest::GetPathArg(size_t index) const {
  return impl_.GetPathArg(index);
}

bool HttpRequest::HasPathArg(const std::string& arg_name) const {
  return impl_.HasPathArg(arg_name);
}

bool HttpRequest::HasPathArg(size_t index) const {
  return impl_.HasPathArg(index);
}

size_t HttpRequest::PathArgCount() const { return impl_.PathArgCount(); }

const std::string& HttpRequest::GetHeader(std::string_view header_name) const {
  return impl_.GetHeader(header_name);
}

const std::string& HttpRequest::GetHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name)
    const {
  return impl_.GetHeader(header_name);
}

bool HttpRequest::HasHeader(std::string_view header_name) const {
  return impl_.HasHeader(header_name);
}

bool HttpRequest::HasHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name)
    const {
  return impl_.HasHeader(header_name);
}

size_t HttpRequest::HeaderCount() const { return impl_.HeaderCount(); }

HttpRequest::HeadersMapKeys HttpRequest::GetHeaderNames() const {
  return impl_.GetHeaderNames();
}

void HttpRequest::RemoveHeader(std::string_view header_name) {
  impl_.RemoveHeader(header_name);
}

void HttpRequest::RemoveHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
  impl_.RemoveHeader(header_name);
}

const std::string& HttpRequest::GetCookie(
    const std::string& cookie_name) const {
  return impl_.GetCookie(cookie_name);
}

bool HttpRequest::HasCookie(const std::string& cookie_name) const {
  return impl_.HasCookie(cookie_name);
}

size_t HttpRequest::CookieCount() const { return impl_.CookieCount(); }

HttpRequest::CookiesMapKeys HttpRequest::GetCookieNames() const {
  return impl_.GetCookieNames();
}

const std::string& HttpRequest::RequestBody() const {
  return impl_.RequestBody();
}

const HttpRequest::HeadersMap& HttpRequest::RequestHeaders() const {
  return impl_.GetHeaders();
}

const HttpRequest::CookiesMap& HttpRequest::RequestCookies() const {
  return impl_.GetCookies();
}

void HttpRequest::SetRequestBody(std::string body) {
  impl_.SetRequestBody(std::move(body));
}  // namespace server::http

void HttpRequest::ParseArgsFromBody() { impl_.ParseArgsFromBody(); }

std::chrono::steady_clock::time_point HttpRequest::GetStartTime() const {
  return impl_.StartTime();
}

void HttpRequest::SetResponseStatus(HttpStatus status) const {
  return impl_.SetResponseStatus(status);
}

bool HttpRequest::IsBodyCompressed() const { return impl_.IsBodyCompressed(); }

}  // namespace server::http

USERVER_NAMESPACE_END
