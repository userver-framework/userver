#pragma once

/// @file userver/clients/http/response.hpp
/// @brief @copybrief clients::http::Response

#include <string>

#include <userver/clients/http/error.hpp>
#include <userver/clients/http/expected_status_codes.hpp>
#include <userver/clients/http/local_stats.hpp>
#include <userver/http/header_map.hpp>
#include <userver/http/status_code.hpp>
#include <userver/server/http/http_response_cookie.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

using Status = USERVER_NAMESPACE::http::StatusCode;

/// Headers container type
using Headers = USERVER_NAMESPACE::http::headers::HeaderMap;

/// Class that will be returned for successful request
class Response final {
public:
    using CookiesMap = server::http::Cookie::CookiesMap;

    Response() = default;

    /// response string
    std::string& sink_string() { return response_; }

    /// body as string
    std::string body() const& { return response_; }
    std::string&& body() && { return std::move(response_); }

    /// body as string_view
    std::string_view body_view() const { return response_; }

    /// return reference to headers
    const Headers& headers() const { return headers_; }
    Headers& headers() { return headers_; }
    const CookiesMap& cookies() const { return cookies_; }
    CookiesMap& cookies() { return cookies_; }

    /// status_code
    Status status_code() const;
    /// check status code
    bool IsOk() const { return status_code() == Status::kOk; }
    /// @brief Whether the HTTP status is >=400 and was not explicitly marked expected by the request.
    /// This classification does not affect raise_for_status().
    bool IsError() const {
        return static_cast<uint16_t>(status_code()) >= 400 && !expected_status_codes_.Contains(status_code());
    }

    static void RaiseForStatus(int code, const LocalStats& stats);
    static void RaiseForStatus(int code, const LocalStats& stats, std::string_view message);

    /// @brief Configuration whether to include the response body in the exception in @ref raise_for_status.
    enum class RaiseIncludeBody : std::uint8_t { kNo = 0, kYes = 1 };

    /// @brief Raise an exception depending on the response status.
    ///        The body of the response may be included in the exception depending on the \c include_body.
    ///
    /// @throws HttpClientException for statuses [400; 500)
    /// @throws HttpServerException for statuses [500; 600)
    void raise_for_status(RaiseIncludeBody include_body = RaiseIncludeBody::kNo) const;

    /// returns statistics on request execution like count of opened sockets, connect time...
    LocalStats GetStats() const;

    void SetStats(const LocalStats& stats) { stats_ = stats; }
    void SetStatusCode(Status status_code) { status_code_ = status_code; }

    /// @brief Replaces the expected HTTP codes used by @ref clients::http::Response::IsError.
    /// An empty set restores the default classification of HTTP statuses >=400 as errors.
    /// The request applies its configured set before completion middleware runs.
    /// Changing a received response affects subsequent response classification only. It does not
    /// update the already recorded span or retry-budget accounting, change the HTTP status,
    /// or affect @ref clients::http::Response::raise_for_status.
    /// To configure request tracing and retry-budget accounting, use
    /// @ref clients::http::Request::SetExpectedStatusCodes before sending the request.
    void SetExpectedStatusCodes(ExpectedStatusCodes status_codes) { expected_status_codes_ = status_codes; }

private:
    Headers headers_;
    CookiesMap cookies_;
    std::string response_;
    Status status_code_{Status::kInvalid};
    ExpectedStatusCodes expected_status_codes_;
    LocalStats stats_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
