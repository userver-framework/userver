#include "http_request_impl.hpp"

#include <engine/task/task.hpp>
#include <logging/logger.hpp>
#include <yandex/taxi/userver/utils/encoding/tskv.hpp>

namespace {

const std::string kHostHeader = "Host";

std::string EscapeLogString(const std::string& str,
                            const std::vector<uint8_t>& need_escape_map) {
  size_t esc_cnt = 0;
  for (char ch : str) {
    if (need_escape_map[static_cast<uint8_t>(ch)]) esc_cnt++;
  }
  if (!esc_cnt) return str;
  std::string res;
  res.reserve(str.size() + esc_cnt * 3);
  for (char ch : str) {
    if (need_escape_map[static_cast<uint8_t>(ch)]) {
      res += '\\';
      res += 'x';
      res += ("0123456789ABCDEF"[(ch >> 4) & 0xF]);
      res += ("0123456789ABCDEF"[ch & 0xF]);
    } else {
      res += ch;
    }
  }
  return res;
}

std::string EscapeForAccessLog(const std::string& str) {
  static auto prepare_need_escape = []() {
    std::vector<uint8_t> res(256, 0);
    for (int i = 0; i < 32; i++) res[i] = 1;
    for (int i = 127; i < 256; i++) res[i] = 1;
    res[static_cast<uint8_t>('\\')] = 1;
    res[static_cast<uint8_t>('"')] = 1;
    return res;
  };

  static const std::vector<uint8_t> kNeedEscape = prepare_need_escape();

  if (str.empty()) return "-";
  return EscapeLogString(str, kNeedEscape);
}

std::string EscapeForAccessTskvLog(const std::string& str) {
  if (str.empty()) return "-";

  std::string encoded_str;
  EncodeTskv(encoded_str, str, utils::encoding::EncodeTskvMode::kValue);
  return encoded_str;
}

const std::string kEmptyString{};
const std::vector<std::string> kEmptyVector{};

}  // namespace

namespace server {
namespace http {

HttpRequestImpl::HttpRequestImpl()
    : http_major_(0),
      http_minor_(0),
      response_(std::make_unique<HttpResponse>(*this)) {}

HttpRequestImpl::~HttpRequestImpl() {}

std::chrono::duration<double> HttpRequestImpl::GetRequestTime() const {
  return GetResponse().SentTime() - StartTime();
}

std::chrono::duration<double> HttpRequestImpl::GetResponseTime() const {
  return GetResponse().ReadyTime() - StartTime();
}

const std::string& HttpRequestImpl::GetHost() const {
  return GetHeader(kHostHeader);
}

const std::string& HttpRequestImpl::GetArg(const std::string& arg_name) const {
  auto it = request_args_.find(arg_name);
  if (it == request_args_.end()) return kEmptyString;
  return it->second.at(0);
}

const std::vector<std::string>& HttpRequestImpl::GetArgVector(
    const std::string& arg_name) const {
  auto it = request_args_.find(arg_name);
  if (it == request_args_.end()) return kEmptyVector;
  return it->second;
}

bool HttpRequestImpl::HasArg(const std::string& arg_name) const {
  auto it = request_args_.find(arg_name);
  return (it != request_args_.end());
}

size_t HttpRequestImpl::ArgCount() const { return request_args_.size(); }

std::vector<std::string> HttpRequestImpl::ArgNames() const {
  std::vector<std::string> res;
  res.reserve(request_args_.size());
  for (const auto& arg : request_args_) res.push_back(arg.first);
  return res;
}

const std::string& HttpRequestImpl::GetHeader(
    const std::string& header_name) const {
  auto it = headers_.find(header_name);
  if (it == headers_.end()) return kEmptyString;
  return it->second;
}

bool HttpRequestImpl::HasHeader(const std::string& header_name) const {
  auto it = headers_.find(header_name);
  return (it != headers_.end());
}

size_t HttpRequestImpl::HeaderCount() const { return headers_.size(); }

HeadersMapKeys HttpRequestImpl::GetHeaderNames() const {
  return headers_ | boost::adaptors::map_keys;
}

const std::string& HttpRequestImpl::GetCookie(
    const std::string& cookie_name) const {
  auto it = cookies_.find(cookie_name);
  if (it == cookies_.end()) return kEmptyString;
  return it->second;
}

bool HttpRequestImpl::HasCookie(const std::string& cookie_name) const {
  return cookies_.count(cookie_name);
}

size_t HttpRequestImpl::CookieCount() const { return cookies_.size(); }

CookiesMapKeys HttpRequestImpl::GetCookieNames() const {
  return cookies_ | boost::adaptors::map_keys;
}

void HttpRequestImpl::SetMatchedPathLength(size_t length) {
  path_suffix_ = request_path_.substr(length);
}

void HttpRequestImpl::WriteAccessLogs(
    const logging::LoggerPtr& logger_access,
    const logging::LoggerPtr& logger_access_tskv,
    const std::string& remote_address) const {
  WriteAccessLog(logger_access, remote_address);
  WriteAccessTskvLog(logger_access_tskv, remote_address);
}

void HttpRequestImpl::WriteAccessLog(const logging::LoggerPtr& logger_access,
                                     const std::string& remote_address) const {
  if (!logger_access) return;

  logger_access->info(
      "{} {} \"{} {} HTTP/{}.{}\" {} \"{}\" \"{}\" \"{}\" {:0.6f} - {} {:0.6f}",
      EscapeForAccessLog(GetHost()), EscapeForAccessLog(remote_address),
      EscapeForAccessLog(GetMethodStr()), EscapeForAccessLog(GetUrl()),
      GetHttpMajor(), GetHttpMinor(), static_cast<int>(response_->GetStatus()),
      EscapeForAccessLog(GetHeader("Referer")),
      EscapeForAccessLog(GetHeader("User-Agent")),
      EscapeForAccessLog(GetHeader("Cookie")), GetRequestTime().count(),
      GetResponse().BytesSent(), GetResponseTime().count());
}

void HttpRequestImpl::WriteAccessTskvLog(
    const logging::LoggerPtr& logger_access_tskv,
    const std::string& remote_address) const {
  if (!logger_access_tskv) return;

  logger_access_tskv->info(
      "\tstatus={}"
      "\tprotocol=HTTP/{}.{}"
      "\tmethod={}"
      "\trequest={}"
      "\treferer={}"
      "\tcookies={}"
      "\tuser_agent={}"
      "\tvhost={}"
      "\tip={}"
      "\tx_forwarded_for={}"
      "\tx_real_ip={}"
      "\tupstream_http_x_yarequestid={}"
      "\thttp_host={}"
      "\tremote_addr={}"
      "\trequest_time={:0.3f}"
      "\tupstream_response_time={:0.3f}"
      "\trequest_body={}",
      static_cast<int>(response_->GetStatus()), GetHttpMajor(), GetHttpMinor(),
      EscapeForAccessTskvLog(GetMethodStr()), EscapeForAccessTskvLog(GetUrl()),
      EscapeForAccessTskvLog(GetHeader("Referer")),
      EscapeForAccessTskvLog(GetHeader("Cookie")),
      EscapeForAccessTskvLog(GetHeader("User-Agent")),
      EscapeForAccessTskvLog(GetHost()), EscapeForAccessTskvLog(remote_address),
      EscapeForAccessTskvLog(GetHeader("X-Forwarded-For")),
      EscapeForAccessTskvLog(GetHeader("X-Real-IP")),
      EscapeForAccessTskvLog(GetHeader("X-YaRequestId")),
      EscapeForAccessTskvLog(GetHost()), EscapeForAccessTskvLog(remote_address),
      GetRequestTime().count(), GetResponseTime().count(),
      EscapeForAccessTskvLog(RequestBody()));
}

}  // namespace http
}  // namespace server
