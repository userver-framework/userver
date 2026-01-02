#pragma once

/// @file userver/clients/http/response.hpp
/// @brief @copybrief clients::http::Response

#include <string>

#include <userver/clients/http/error.hpp>
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
    bool IsError() const { return static_cast<uint16_t>(status_code()) >= 400; }

    static void RaiseForStatus(int code, const LocalStats& stats);
    static void RaiseForStatus(int code, const LocalStats& stats, std::string_view message);

    /// @brief Configuration whether to include the response body in the exception in @ref raise_for_status.
    enum class RaiseIncludeBody : std::uint8_t { kNo = 0, kYes = 1 };

    /// @brief Raise an exception depending on the response status.
    ///        The body of the response may be included in the exception depending on the @param include_body.
    ///
    /// @throws HttpClientException for statuses [400; 500)
    /// @throws HttpServerException for statuses [500; 600)
    void raise_for_status(RaiseIncludeBody include_body = RaiseIncludeBody::kNo) const;

    /// returns statistics on request execution like count of opened sockets, connect time...
    LocalStats GetStats() const;

    void SetStats(const LocalStats& stats) { stats_ = stats; }
    void SetStatusCode(Status status_code) { status_code_ = status_code; }

private:
    Headers headers_;
    CookiesMap cookies_;
    std::string response_;
    Status status_code_{Status::kInvalid};
    LocalStats stats_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
