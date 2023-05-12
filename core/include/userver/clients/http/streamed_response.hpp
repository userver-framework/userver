#pragma once

/// @file userver/clients/http/streamed_response.hpp
/// @brief @copybrief clients::http::StreamedResponse

#include <future>

#include <userver/clients/http/response.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class RequestState;

/// @brief HTTP response for streamed API.
///
/// Call Request::async_perform_stream_body()
/// to get one.  You can use it for fast proxying backend response body
/// to a remote Application.
class StreamedResponse final {
 public:
  StreamedResponse(StreamedResponse&&) = default;
  StreamedResponse(const StreamedResponse&) = delete;

  StreamedResponse& operator=(StreamedResponse&&) = default;
  StreamedResponse& operator=(const StreamedResponse&) = delete;

  /// @brief HTTP status code
  /// @note may suspend the coroutine if the code is not obtained yet.
  Status StatusCode();

  /// Returns HTTP header value by its name (case-insensitive)
  /// A missing key results in empty string
  /// @note may suspend the coroutine if headers are not obtained yet.
  std::string GetHeader(const std::string& header_name);

  /// Get all HTTP headers as a case-insensitive unordered map
  /// @note may suspend the coroutine if headers are not obtained yet.
  const Headers& GetHeaders();
  const Response::CookiesMap& GetCookies();

  using Queue = concurrent::StringStreamQueue;

  /// Read another HTTP response body part into 'output'.
  /// Any previous data in 'output' is dropped.
  /// @note The chunk size is not guaranteed to be exactly
  /// multipart/form-data chunk size or any other HTTP-related size
  /// @note may block if the chunk is not obtained yet.
  bool ReadChunk(std::string& output, engine::Deadline);

  /// Creates a new StreamedResponse
  StreamedResponse(Queue::Consumer&& queue_consumer, engine::Deadline deadline,
                   std::shared_ptr<clients::http::RequestState> request_state);

 private:
  std::future_status WaitForHeaders(engine::Deadline);
  void WaitForHeadersOrThrow(engine::Deadline);

  std::shared_ptr<RequestState> request_state_;
  std::shared_ptr<Response>
      response_;  // re-use sync response's headers & status code storage
  engine::Deadline deadline_;

  Queue::Consumer queue_consumer_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
