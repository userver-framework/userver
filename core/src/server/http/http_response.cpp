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

#include <server/http/http_cached_date.hpp>

#include "http_request_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kCrlf = "\r\n";
constexpr std::string_view kKeyValueHeaderSeparator = ": ";

const auto kDefaultContentTypeString =
    http::ContentType{"text/html; charset=utf-8"}.ToString();

constexpr std::string_view kClose = "close";
constexpr std::string_view kKeepAlive = "keep-alive";

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

bool IsBodyForbiddenForStatus(server::http::HttpStatus status) {
  return status == server::http::HttpStatus::kNoContent ||
         status == server::http::HttpStatus::kNotModified ||
         (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
}

}  // namespace

namespace server::http {

namespace impl {

void OutputHeader(std::string& header, std::string_view key,
                  std::string_view val) {
  const auto old_size = header.size();
  header.resize(old_size + key.size() + kKeyValueHeaderSeparator.size() +
                val.size() + kCrlf.size());

  char* append_position = header.data() + old_size;
  const auto append = [&append_position](std::string_view what) {
    std::memcpy(append_position, what.data(), what.size());
    append_position += what.size();
  };

  append(key);
  append(kKeyValueHeaderSeparator);
  append(val);
  append(kCrlf);
}

}  // namespace impl

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
  // According to https://www.chromium.org/spdy/spdy-whitepaper/
  // "typical header sizes of 700-800 bytes is common"
  // Adjusting it to 1KiB to fit jemalloc size class
  static constexpr auto kTypicalHeadersSize = 1024;

  std::string header;
  header.reserve(kTypicalHeadersSize);

  header.append("HTTP/");
  fmt::format_to(std::back_inserter(header), FMT_COMPILE("{}.{} {} "),
                 request_.GetHttpMajor(), request_.GetHttpMinor(),
                 static_cast<int>(status_));
  header.append(HttpStatusString(status_));
  header.append(kCrlf);

  headers_.erase(USERVER_NAMESPACE::http::headers::kContentLength);
  const auto end = headers_.cend();
  if (headers_.find(USERVER_NAMESPACE::http::headers::kDate) == end) {
    header.append(USERVER_NAMESPACE::http::headers::kDate);
    header.append(kKeyValueHeaderSeparator);
    AppendCachedDate(header);
    header.append(kCrlf);
  }
  if (headers_.find(USERVER_NAMESPACE::http::headers::kContentType) == end) {
    impl::OutputHeader(header, USERVER_NAMESPACE::http::headers::kContentType,
                       kDefaultContentTypeString);
  }
  for (const auto& item : headers_) {
    impl::OutputHeader(header, item.first, item.second);
  }
  if (headers_.find(USERVER_NAMESPACE::http::headers::kConnection) == end) {
    impl::OutputHeader(header, USERVER_NAMESPACE::http::headers::kConnection,
                       (request_.IsFinal() ? kClose : kKeepAlive));
  }
  for (const auto& cookie : cookies_) {
    header.append(USERVER_NAMESPACE::http::headers::kSetCookie);
    header.append(kKeyValueHeaderSeparator);
    cookie.second.AppendToString(header);
    header.append(kCrlf);
  }

  if (IsBodyStreamed() && GetData().empty()) {
    SetBodyStreamed(socket, header);
  } else {
    // e.g. a CustomHandlerException
    SetBodyNotstreamed(socket, header);
  }
}

void HttpResponse::SetBodyNotstreamed(engine::io::Socket& socket,
                                      std::string& header) {
  const bool is_body_forbidden = IsBodyForbiddenForStatus(status_);
  const bool is_head_request = request_.GetOrigMethod() == HttpMethod::kHead;
  const auto& data = GetData();

  if (!is_body_forbidden) {
    impl::OutputHeader(header, USERVER_NAMESPACE::http::headers::kContentLength,
                       fmt::format(FMT_COMPILE("{}"), data.size()));
  }
  header.append(kCrlf);

  if (is_body_forbidden && !data.empty()) {
    LOG_LIMITED_WARNING()
        << "Non-empty body provided for response with HTTP code "
        << static_cast<int>(status_)
        << " which does not allow one, it will be dropped";
  }

  ssize_t sent_bytes = 0;
  if (!is_head_request && !is_body_forbidden) {
    sent_bytes = socket.SendAll(
        {{header.data(), header.size()}, {data.data(), data.size()}},
        engine::Deadline{});
  } else {
    sent_bytes =
        socket.SendAll(header.data(), header.size(), engine::Deadline{});
  }

  SetSentTime(std::chrono::steady_clock::now());
  SetSent(sent_bytes);
}

void HttpResponse::SetBodyStreamed(engine::io::Socket& socket,
                                   std::string& header) {
  impl::OutputHeader(
      header, USERVER_NAMESPACE::http::headers::kTransferEncoding, "chunked");

  // send HTTP headers
  size_t sent_bytes = socket.SendAll(header.data(), header.size(), {});
  std::string().swap(header);  // free memory before time consuming operation

  // Transmit HTTP response body
  std::string body_part;
  while (body_stream_->Pop(body_part)) {
    if (body_part.empty()) {
      LOG_DEBUG() << "Zero size body_part in http_response.cpp";
      continue;
    }

    auto size = fmt::format("\r\n{:x}\r\n", body_part.size());
    sent_bytes += socket.SendAll(
        {{size.data(), size.size()}, {body_part.data(), body_part.size()}},
        engine::Deadline{});
  }

  const constexpr std::string_view terminating_chunk{"\r\n0\r\n\r\n"};
  sent_bytes +=
      socket.SendAll(terminating_chunk.data(), terminating_chunk.size(), {});

  // TODO: exceptions?
  body_stream_producer_.reset();
  body_stream_.reset();

  SetSentTime(std::chrono::steady_clock::now());
  SetSent(sent_bytes);
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
