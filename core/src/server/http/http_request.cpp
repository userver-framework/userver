#include <server/http/http_request.hpp>

#include "http_request_impl.hpp"

namespace server::http {

HttpRequest::HttpRequest(const HttpRequestImpl& impl) : impl_(impl) {}

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

const std::string& HttpRequest::GetHeader(
    const std::string& header_name) const {
  return impl_.GetHeader(header_name);
}

bool HttpRequest::HasHeader(const std::string& header_name) const {
  return impl_.HasHeader(header_name);
}

size_t HttpRequest::HeaderCount() const { return impl_.HeaderCount(); }

HttpRequest::HeadersMapKeys HttpRequest::GetHeaderNames() const {
  return impl_.GetHeaderNames();
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

void HttpRequest::SetResponseStatus(HttpStatus status) const {
  return impl_.SetResponseStatus(status);
}

}  // namespace server::http
