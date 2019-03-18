#include <server/http/http_response.hpp>

#include <iomanip>
#include <sstream>

#include <cctz/time_zone.h>

#include <build_config.hpp>
#include <engine/io/socket.hpp>
#include <http/common_headers.hpp>
#include <server/http/content_type.hpp>

#include "http_request_impl.hpp"

namespace {

const std::string kCrlf = "\r\n";
const std::string kResponseHttpVersionPrefix = "HTTP/";
const std::string kServerName =
    std::string(::http::headers::kServer) + ": taxi_userver/" USERVER_VERSION;

const std::string kClose = "close";
const std::string kKeepAlive = "keep-alive";

void CheckHeaderName(const std::string& name) {
  static auto init = []() {
    std::vector<uint8_t> res(256, 0);
    for (int i = 0; i < 32; i++) res[i] = 1;
    for (int i = 127; i < 256; i++) res[i] = 1;
    for (char c : std::string("()<>@,;:\\\"/[]?={} \t"))
      res[static_cast<int>(c)] = 1;
    return res;
  };
  static auto bad_chars = init();

  for (char c : name) {
    uint8_t code = static_cast<uint8_t>(c);
    if (bad_chars[code]) {
      throw std::runtime_error(
          std::string("invalid character in header name: '") + c + "' (#" +
          std::to_string(code) + ")");
    }
  }
}

void CheckHeaderValue(const std::string& value) {
  static auto init = []() {
    std::vector<uint8_t> res(256, 0);
    for (int i = 0; i < 32; i++) res[i] = 1;
    res[127] = 1;
    return res;
  };
  static auto bad_chars = init();

  for (char c : value) {
    uint8_t code = static_cast<uint8_t>(c);
    if (bad_chars[code]) {
      throw std::runtime_error(
          std::string("invalid character in header value: '") + c + "' (#" +
          std::to_string(code) + ")");
    }
  }
}

}  // namespace

namespace server {
namespace http {

HttpResponse::HttpResponse(const HttpRequestImpl& request)
    : request_(request) {}

HttpResponse::~HttpResponse() {}

void HttpResponse::SetSendFailed(
    std::chrono::steady_clock::time_point failure_time) {
  SetStatus(HttpStatus::kClientClosedRequest);
  request::ResponseBase::SetSendFailed(failure_time);
}

void HttpResponse::SetHeader(std::string name, std::string value) {
  CheckHeaderName(name);
  CheckHeaderValue(value);
  headers_.emplace(std::move(name), std::move(value));
}

void HttpResponse::SetContentType(std::string type) {
  SetHeader(::http::headers::kContentType, std::move(type));
}

void HttpResponse::SetContentEncoding(std::string encoding) {
  SetHeader(::http::headers::kContentEncoding, std::move(encoding));
}

void HttpResponse::SetStatus(HttpStatus status) { status_ = status; }

void HttpResponse::ClearHeaders() { headers_.clear(); }

HttpResponse::HeadersMapKeys HttpResponse::GetHeaderNames() const {
  return headers_ | boost::adaptors::map_keys;
}

const std::string& HttpResponse::GetHeader(
    const std::string& header_name) const {
  return headers_.at(header_name);
}

void HttpResponse::SendResponse(engine::io::Socket& socket) {
  bool is_head_request = request_.GetOrigMethod() == HttpMethod::kHead;
  std::ostringstream os;
  os << kResponseHttpVersionPrefix << request_.GetHttpMajor() << '.'
     << request_.GetHttpMinor() << ' ' << static_cast<int>(status_) << ' '
     << HttpStatusString(status_) << kCrlf;
  os << kServerName << kCrlf;
  static const auto format_string = "%a, %d %b %Y %H:%M:%S %Z";
  static const auto tz = cctz::utc_time_zone();
  const auto& time_str =
      cctz::format(format_string, std::chrono::system_clock::now(), tz);

  os << "Date: " << time_str << kCrlf;
  if (headers_.find(::http::headers::kContentType) == headers_.end())
    os << ::http::headers::kContentType << ": " << content_type::kTextHtml
       << kCrlf;
  for (const auto& header : headers_)
    os << header.first << ": " << header.second << kCrlf;
  if (headers_.find(::http::headers::kConnection) == headers_.end())
    os << ::http::headers::kConnection << ": "
       << (request_.IsFinal() ? kClose : kKeepAlive) << kCrlf;
  os << "Content-Length: " << data_.size() << kCrlf << kCrlf;
  if (!is_head_request) os << data_;

  const auto response_data = os.str();
  auto sent_bytes =
      socket.SendAll(response_data.data(), response_data.size(), {});
  SetSentTime(std::chrono::steady_clock::now());
  SetSent(sent_bytes);
}

}  // namespace http
}  // namespace server
