#include <userver/clients/http/expected_status_codes.hpp>

#include <stdexcept>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

ExpectedStatusCodes::ExpectedStatusCodes(std::initializer_list<USERVER_NAMESPACE::http::StatusCode> status_codes) {
    for (const auto status : status_codes) {
        Set(status, true);
    }
}

void ExpectedStatusCodes::Set(USERVER_NAMESPACE::http::StatusCode status, bool expected) {
    const auto status_code = static_cast<int>(status);
    if (status_code < kMinHttpStatusCode || status_code > kMaxHttpStatusCode) {
        throw std::invalid_argument(fmt::format(
            "Expected HTTP status code {} is outside [{}, {}]. Pass valid HTTP status codes.",
            status_code,
            kMinHttpStatusCode,
            kMaxHttpStatusCode
        ));
    }
    status_codes_.set(static_cast<std::size_t>(status_code - kMinHttpStatusCode), expected);
}

bool ExpectedStatusCodes::Contains(USERVER_NAMESPACE::http::StatusCode status_code) const {
    const auto code = static_cast<int>(status_code);
    return code >= kMinHttpStatusCode && code <= kMaxHttpStatusCode &&
           status_codes_.test(static_cast<std::size_t>(code - kMinHttpStatusCode));
}

}  // namespace clients::http

USERVER_NAMESPACE_END
