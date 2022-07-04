#include <userver/server/http/http_response.hpp>

#include <array>

#include <cctz/time_zone.h>
#include <fmt/compile.h>

#include <userver/engine/io/socket.hpp>
#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/set_throttle_reason.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/userver_info.hpp>

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

void OutputHeader(std::string& os, std::string_view key, std::string_view val) {
  os.append(key);
  os.append(kKeyValueHeaderSeparator);
  os.append(val);
  os.append(kCrlf);
}

}  // namespace impl

HttpResponse::HttpResponse(const HttpRequestImpl& request,
                           request::ResponseDataAccounter& data_accounter)
    : ResponseBase(data_accounter), request_(request) {}

HttpResponse::~HttpResponse() = default;

void HttpResponse::SetSendFailed(
    std::chrono::steady_clock::time_point failure_time) {
  SetStatus(HttpStatus::kClientClosedRequest);
  request::ResponseBase::SetSendFailed(failure_time);
}

void HttpResponse::SetHeader(std::string name, std::string value) {
  CheckHeaderName(name);
  CheckHeaderValue(value);
  const auto header_it = headers_.find(name);
  if (header_it == headers_.end()) {
    headers_.emplace(std::move(name), std::move(value));
  } else {
    header_it->second = std::move(value);
  }
}

void HttpResponse::SetContentType(
    const USERVER_NAMESPACE::http::ContentType& type) {
  SetHeader(USERVER_NAMESPACE::http::headers::kContentType, type.ToString());
}

void HttpResponse::SetContentEncoding(std::string encoding) {
  SetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding,
            std::move(encoding));
}

void HttpResponse::SetStatus(HttpStatus status) { status_ = status; }

void HttpResponse::ClearHeaders() { headers_.clear(); }

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
  return cookies_.at(cookie_name);
}

void HttpResponse::SendResponse(engine::io::Socket& socket) {
  const bool is_head_request = request_.GetOrigMethod() == HttpMethod::kHead;
  const bool is_body_forbidden = IsBodyForbiddenForStatus(status_);
  const auto& data = GetData();

  static constexpr auto kMinSeparateDataSize = 50000;  //  50KB
  bool separate_data_send = data.size() > kMinSeparateDataSize;

  // According to https://www.chromium.org/spdy/spdy-whitepaper/
  // "typical header sizes of 700-800 bytes is common"
  // Adjusting it to 1KiB to fit jemalloc size class
  static constexpr auto kTypicalHeadersSize = 1024;

  std::string os;
  if (!is_body_forbidden && !separate_data_send && !is_head_request) {
    os.reserve(kTypicalHeadersSize + data.size());
  } else {
    os.reserve(kTypicalHeadersSize);
  }

  os.append("HTTP/");
  fmt::format_to(std::back_inserter(os), FMT_COMPILE("{}.{} {} "),
                 request_.GetHttpMajor(), request_.GetHttpMinor(),
                 static_cast<int>(status_));
  os.append(HttpStatusString(status_));
  os.append(kCrlf);

  headers_.erase(USERVER_NAMESPACE::http::headers::kContentLength);
  const auto end = headers_.cend();
  if (headers_.find(USERVER_NAMESPACE::http::headers::kDate) == end) {
    static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
    static const auto tz = cctz::utc_time_zone();
    const auto& time_str =
        cctz::format(kFormatString, std::chrono::system_clock::now(), tz);

    impl::OutputHeader(os, USERVER_NAMESPACE::http::headers::kDate, time_str);
  }
  if (headers_.find(USERVER_NAMESPACE::http::headers::kContentType) == end) {
    impl::OutputHeader(os, USERVER_NAMESPACE::http::headers::kContentType,
                       kDefaultContentTypeString);
  }
  for (const auto& header : headers_) {
    impl::OutputHeader(os, header.first, header.second);
  }
  if (headers_.find(USERVER_NAMESPACE::http::headers::kConnection) == end) {
    impl::OutputHeader(os, USERVER_NAMESPACE::http::headers::kConnection,
                       (request_.IsFinal() ? kClose : kKeepAlive));
  }
  if (!is_body_forbidden) {
    impl::OutputHeader(os, USERVER_NAMESPACE::http::headers::kContentLength,
                       fmt::format(FMT_COMPILE("{}"), data.size()));
  }
  for (const auto& cookie : cookies_) {
    os.append(USERVER_NAMESPACE::http::headers::kSetCookie);
    os.append(kKeyValueHeaderSeparator);
    cookie.second.AppendToString(os);
    os.append(kCrlf);
  }
  os.append(kCrlf);

  if (!is_body_forbidden) {
    if (!separate_data_send && !is_head_request) {
      os.append(data);
    }
  } else {
    separate_data_send = false;
    if (!data.empty()) {
      LOG_LIMITED_WARNING()
          << "Non-empty body provided for response with HTTP code "
          << static_cast<int>(status_)
          << " which does not allow one, it will be dropped";
    }
  }

  auto sent_bytes = socket.SendAll(os.data(), os.size(), {});

  if (separate_data_send && sent_bytes == os.size() && !is_head_request) {
    std::string().swap(os);  // free memory before time consuming operation

    // If response is too big, copying is more expensive than +1 syscall
    sent_bytes += socket.SendAll(data.data(), data.size(), {});
  }

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

}  // namespace server::http

USERVER_NAMESPACE_END
