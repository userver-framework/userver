#include <server/http/http_response.hpp>

#include <iomanip>
#include <sstream>

#include <build_config.hpp>
#include <engine/sender.hpp>
#include "http_request_impl.hpp"

namespace {

const std::string kContentTypeTextHtml = "text/html";
const std::string kCrlf = "\r\n";
const std::string kHeaderContentEncoding = "Content-Encoding";
const std::string kHeaderContentType = "Content-Type";
const std::string kResponseHttpVersion = "HTTP/1.1";
const std::string kServerName = "Server: taxi_userver/" USERVER_VERSION;

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

void HttpResponse::SetSent(size_t bytes_sent) {
  if (!bytes_sent) SetStatus(HttpStatus::kClientClosedRequest);
  request::ResponseBase::SetSent(bytes_sent);
}

void HttpResponse::SetHeader(std::string name, std::string value) {
  CheckHeaderName(name);
  CheckHeaderValue(value);
  headers_.emplace(std::move(name), std::move(value));
}

void HttpResponse::SetContentType(std::string type) {
  SetHeader(kHeaderContentType, std::move(type));
}

void HttpResponse::SetContentEncoding(std::string encoding) {
  SetHeader(kHeaderContentEncoding, std::move(encoding));
}

void HttpResponse::SetStatus(HttpStatus status) { status_ = status; }

void HttpResponse::ClearHeaders() { headers_.clear(); }

void HttpResponse::SendResponse(engine::Sender& sender,
                                std::function<void(size_t)> finish_cb,
                                bool need_send) {
  if (!need_send) {
    finish_cb(0);
    return;
  }
  bool is_head_request = request_.GetMethod() == HttpMethod::kHead;
  std::ostringstream os;
  os << kResponseHttpVersion << " " << static_cast<int>(status_) << " "
     << HttpStatusString(status_) << kCrlf;
  os << kServerName << kCrlf;
  time_t time_t_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  struct tm tm;
  gmtime_r(&time_t_time, &tm);
  os << "Date: " << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z") << kCrlf;
  if (headers_.find(kHeaderContentType) == headers_.end())
    os << kHeaderContentType << ": " << kContentTypeTextHtml << kCrlf;
  for (const auto& header : headers_)
    os << header.first << ": " << header.second << kCrlf;
  os << "Connection: keep-alive" << kCrlf;
  os << "Content-Length: " << data_.size() << kCrlf << kCrlf;
  if (!is_head_request) os << data_;
  sender.SendData(os.str(), std::move(finish_cb));
}

HttpResponse::HeadersMapKeys HttpResponse::GetHeaderNames() const {
  return headers_ | boost::adaptors::map_keys;
}

const std::string& HttpResponse::GetHeader(
    const std::string& header_name) const {
  return headers_.at(header_name);
}

}  // namespace http
}  // namespace server
