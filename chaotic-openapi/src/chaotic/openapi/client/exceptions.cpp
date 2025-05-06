#include <userver/chaotic/openapi/client/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

ExceptionWithStatusCode::ExceptionWithStatusCode(int status_code) : status_code_(status_code) {}

ExceptionWithStatusCode::~ExceptionWithStatusCode() = default;

int ExceptionWithStatusCode::GetStatusCode() const { return status_code_; }

void ExceptionWithStatusCode::SetHeaders(std::unordered_map<std::string, std::string>&& headers) {
    headers_ = std::move(headers);
}

std::string ExceptionWithStatusCode::GetHeader(const std::string& name) const { return headers_.at(name); }

HttpException::HttpException(clients::http::ErrorKind error_kind) : error_kind_(error_kind) {}

HttpException::~HttpException() = default;

clients::http::ErrorKind HttpException::GetErrorKind() const { return error_kind_; }

TimeoutException::~TimeoutException() = default;

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
