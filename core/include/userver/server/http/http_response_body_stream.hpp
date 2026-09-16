#pragma once

/// @file userver/server/http/http_response_body_stream.hpp
/// @brief @copybrief server::http::ResponseBodyStream

#include <string>

#include <userver/server/http/http_response.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief Streaming HTTP response body writer that is passed to
/// @ref server::handlers::HttpHandlerBase::HandleStreamRequest() overloads.
///
/// @see @ref scripts/docs/en/userver/http_server.md
class ResponseBodyStream final {
public:
    /// Constructor that can be used in @ref server::handlers::HttpHandlerBase::HandleHttpRequest
    explicit ResponseBodyStream(HttpResponse& http_response);

    ResponseBodyStream(ResponseBodyStream&&) = default;
    ~ResponseBodyStream();

    /// Send a chunk of response data; should not be called after SetBody().
    /// It may NOT generate exactly one HTTP chunk per call to @ref PushBodyChunk().
    void PushBodyChunk(std::string&& chunk, engine::Deadline deadline);

    /// Set full response body instead of sending chunks; should not be called after @ref PushBodyChunk()
    void SetBody(std::string&& body);

    void SetHeader(const std::string&, const std::string&);

    void SetHeader(std::string_view, const std::string&);

    void SetEndOfHeaders();

    /// Set the HTTP status code; should not be called after @ref PushBodyChunk() as the result will be ignored.
    void SetStatusCode(int status_code);

    /// @overload
    void SetStatusCode(HttpStatus status);

private:
    bool headers_ended_{false};
    bool headers_end_sent_{false};
    HttpResponse::Producer queue_producer_;
    HttpResponse& http_response_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
