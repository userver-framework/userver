#include <userver/server/http/http_response.hpp>

#include <array>

#include <cctz/time_zone.h>
#include <fmt/compile.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/set_throttle_reason.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>

#include "http_response_writer.hpp"

#include "http_request_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kHostname = hostinfo::blocking::GetRealHostName();

void CheckHeaderName(std::string_view name) {
  static constexpr auto init = []() {
    std::array<uint8_t, 256> res{};  // zero initialize
    for (int i = 0; i < 32; i++) res[i] = 1;
    for (int i = 127; i < 256; i++) res[i] = 1;
    for (unsigned char c : "()<>@,;:\\\"/[]?={} \t") res[c] = 1;
    return res;
  };
  static constexpr auto bad_chars = init();

  for (char c : name) {
    auto code = static_cast<uint8_t>(c);
    if (bad_chars[code]) {
      throw std::runtime_error(
          std::string("invalid character in header name: '") + c + "' (#" +
          std::to_string(code) + ")");
    }
  }
}

void CheckHeaderValue(std::string_view value) {
  for (char c : value) {
    auto code = static_cast<uint8_t>(c);
    if (code < 32 || code == 127) {
      throw std::runtime_error(
          std::string("invalid character in header value: '") + c + "' (#" +
          std::to_string(code) + ")");
    }
  }
}

}  // namespace

namespace server::http {

HttpResponse::HttpResponse(const HttpRequestImpl& request,
                           request::ResponseDataAccounter& data_accounter)
    : ResponseBase(data_accounter),
      request_(request),
      headers_end_(engine::SingleConsumerEvent::NoAutoReset()) {}

HttpResponse::~HttpResponse() = default;

void HttpResponse::SetSendFailed(
    std::chrono::steady_clock::time_point failure_time) {
  SetStatus(HttpStatus::kClientClosedRequest);
  request::ResponseBase::SetSendFailed(failure_time);
}

bool HttpResponse::SetHeader(std::string name, std::string value) {
  if (headers_end_.IsReady()) {
    // Attempt to set headers for Stream'ed response after it is already set
    return false;
  }

  CheckHeaderName(name);
  CheckHeaderValue(value);
  const auto header_it = headers_.find(name);
  if (header_it == headers_.end()) {
    headers_.emplace(std::move(name), std::move(value));
  } else {
    header_it->second = std::move(value);
  }

  return true;
}

void HttpResponse::SetContentType(
    const USERVER_NAMESPACE::http::ContentType& type) {
  SetHeader(USERVER_NAMESPACE::http::headers::kContentType, type.ToString());
}

void HttpResponse::SetContentEncoding(std::string encoding) {
  SetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding,
            std::move(encoding));
}

bool HttpResponse::SetStatus(HttpStatus status) {
  if (headers_end_.IsReady()) {
    // Attempt to set headers for Stream'ed response after it is already set
    return false;
  }

  status_ = status;
  return true;
}

bool HttpResponse::ClearHeaders() {
  if (headers_end_.IsReady()) {
    // Attempt to set headers for Stream'ed response after it is already set
    return false;
  }

  headers_.clear();
  return true;
}

void HttpResponse::SetCookie(Cookie cookie) {
  CheckHeaderValue(cookie.Name());
  CheckHeaderValue(cookie.Value());
  UASSERT(!cookie.Name().empty());
  auto [it, ok] = cookies_.emplace(std::string_view{}, std::move(cookie));
  UASSERT(ok);

  auto node = cookies_.extract(it);
  UASSERT(node);
  node.key() = node.mapped().Name();
  cookies_.insert(std::move(node));
}

void HttpResponse::ClearCookies() { cookies_.clear(); }

HttpResponse::HeadersMapKeys HttpResponse::GetHeaderNames() const {
  return HttpResponse::HeadersMapKeys{headers_};
}

const std::string& HttpResponse::GetHeader(
    const std::string& header_name) const {
  return headers_.at(header_name);
}

bool HttpResponse::HasHeader(const std::string& header_name) const {
  return headers_.find(header_name) != headers_.end();
}

HttpResponse::CookiesMapKeys HttpResponse::GetCookieNames() const {
  return HttpResponse::CookiesMapKeys{cookies_};
}

const Cookie& HttpResponse::GetCookie(std::string_view cookie_name) const {
  return cookies_.at(cookie_name.data());
}

void HttpResponse::SetHeadersEnd() { headers_end_.Send(); }

bool HttpResponse::WaitForHeadersEnd() { return headers_end_.WaitForEvent(); }

void HttpResponse::SendResponse(engine::io::Socket& socket) {
  WriteHttp1Response(socket, *this);
}

void HttpResponse::SendResponseHttp2(
    engine::io::Socket& socket,
    concurrent::Variable<impl::SessionPtr>& session_holder,
    std::optional<Http2UpgradeData> upgrade_data) {
  WriteHttp2Response(socket, *this, session_holder, std::move(upgrade_data));
}

void SetThrottleReason(http::HttpResponse& http_response,
                       std::string log_reason, std::string http_header_reason) {
  http_response.SetHeader(
      USERVER_NAMESPACE::http::headers::kXYaTaxiRatelimitedBy, kHostname);
  http_response.SetHeader(
      USERVER_NAMESPACE::http::headers::kXYaTaxiRatelimitReason,
      std::move(http_header_reason));

  if (auto* span = tracing::Span::CurrentSpanUnchecked()) {
    tracing::SetThrottleReason(*span, std::move(log_reason));
  }
}

void HttpResponse::SetStreamBody() {
  UASSERT(!body_stream_);

  const auto body_queue = Queue::Create();
  body_stream_.emplace(body_queue->GetConsumer());
  body_stream_producer_.emplace(body_queue->GetProducer());
}

bool HttpResponse::IsBodyStreamed() const { return body_stream_.has_value(); }

HttpResponse::Queue::Producer HttpResponse::GetBodyProducer() {
  UASSERT(IsBodyStreamed());
  UASSERT_MSG(body_stream_producer_, "GetBodyProducer() is called twice");

  auto producer = std::move(*body_stream_producer_);
  body_stream_producer_.reset();  // don't leave engaged moved-from state
  return producer;
}

}  // namespace server::http

USERVER_NAMESPACE_END
