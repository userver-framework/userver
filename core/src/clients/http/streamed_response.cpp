#include <clients/http/request_state.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

StreamedResponse::StreamedResponse(Queue::Consumer&& queue_consumer,
                                   engine::Deadline deadline,
                                   std::shared_ptr<RequestState> request_state)
    : request_state_(std::move(request_state)),
      deadline_(deadline),
      queue_consumer_(std::move(queue_consumer)) {
  UASSERT(std::get_if<RequestState::StreamData>(&request_state_->data_));
}

std::future_status StreamedResponse::WaitForHeaders(engine::Deadline deadline) {
  if (response_) {
    LOG_DEBUG() << "WaitForHeaders() reply is cached";
    return std::future_status::ready;
  }

  auto* stream_data =
      std::get_if<RequestState::StreamData>(&request_state_->data_);
  UASSERT(stream_data);

  auto& future = stream_data->headers_future;
  auto status = future.wait_until(deadline);
  if (status == engine::FutureStatus::kTimeout) {
    LOG_DEBUG() << "WaitForHeaders is timeouted";
    return std::future_status::timeout;
  }
  future.get();  // maybe throws an exception

  response_ = request_state_->response();
  UASSERT(response_);
  return std::future_status::ready;
}

void StreamedResponse::WaitForHeadersOrThrow(engine::Deadline deadline) {
  auto status = WaitForHeaders(deadline);
  if (status != std::future_status::ready) {
    throw clients::http::TimeoutException("Timeout on streamed response", {});
  }
}

Status StreamedResponse::StatusCode() {
  WaitForHeadersOrThrow(deadline_);
  return response_->status_code();
}

std::string StreamedResponse::GetHeader(const std::string& header_name) {
  WaitForHeadersOrThrow(deadline_);
  const auto& headers = response_->headers();
  return utils::FindOrDefault(headers, header_name, std::string{});
}

const Headers& StreamedResponse::GetHeaders() {
  WaitForHeadersOrThrow(deadline_);
  const auto& headers = response_->headers();
  return headers;
}

const Response::CookiesMap& StreamedResponse::GetCookies() {
  WaitForHeadersOrThrow(deadline_);
  const auto& cookies = response_->cookies();
  return cookies;
}

bool StreamedResponse::ReadChunk(std::string& output,
                                 engine::Deadline deadline) {
  WaitForHeadersOrThrow(deadline_);

  return queue_consumer_.Pop(output, deadline);
}

}  // namespace clients::http

USERVER_NAMESPACE_END
