#include <userver/clients/http/response.hpp>

#include <fmt/core.h>

#include <userver/clients/http/response_future.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

Status Response::status_code() const { return status_code_; }

void Response::RaiseForStatus(int code, const LocalStats& stats) {
    if (400 <= code && code < 500) {
        throw HttpClientException(code, stats);
    } else if (500 <= code && code < 600) {
        throw HttpServerException(code, stats);
    }
}

void Response::RaiseForStatus(int code, const LocalStats& stats, std::string_view message) {
    if (400 <= code && code < 500) {
        throw HttpClientException(code, stats, message);
    } else if (500 <= code && code < 600) {
        throw HttpServerException(code, stats, message);
    }
}

void Response::raise_for_status(RaiseIncludeBody include_body) const {
    switch (include_body) {
        case RaiseIncludeBody::kNo:
            RaiseForStatus(status_code(), GetStats());
            return;
        case RaiseIncludeBody::kYes:
            RaiseForStatus(status_code(), GetStats(), body());
            return;
    }
    UINVARIANT(false, fmt::format("Unhandled 'include_body' value: {}", static_cast<int>(include_body)));
}

LocalStats Response::GetStats() const { return stats_; }

}  // namespace clients::http

USERVER_NAMESPACE_END
