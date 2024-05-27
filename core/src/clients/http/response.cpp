#include <userver/clients/http/response.hpp>

#include <userver/clients/http/response_future.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

Status Response::status_code() const { return status_code_; }

void Response::RaiseForStatus(int code, const LocalStats& stats) {
  if (400 <= code && code < 500)
    throw HttpClientException(code, stats);
  else if (500 <= code && code < 600)
    throw HttpServerException(code, stats);
}

void Response::raise_for_status() const {
  RaiseForStatus(status_code(), GetStats());
}

LocalStats Response::GetStats() const { return stats_; }

}  // namespace clients::http

USERVER_NAMESPACE_END
