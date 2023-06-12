#include "http_request_impl.hpp"

#include <server/handlers/http_handler_base_statistics.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/parser/http_request_parse_args.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr size_t kBucketCount = 16;

constexpr size_t kZeroAllocationBucketCount = 0;

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

namespace server::http {

// Use hash_function() magic to pass out the same RNG seed among all
// unordered_maps because we don't need different seeds and want to avoid its
// overhead.
HttpRequestImpl::HttpRequestImpl(request::ResponseDataAccounter& data_accounter)
    : form_data_args_(kZeroAllocationBucketCount,
                      request_args_.hash_function()),
      path_args_by_name_index_(kZeroAllocationBucketCount,
                               request_args_.hash_function()),
      headers_(kBucketCount),
      cookies_(kZeroAllocationBucketCount, request_args_.hash_function()),
      response_(*this, data_accounter) {}

HttpRequestImpl::~HttpRequestImpl() = default;

std::chrono::duration<double> HttpRequestImpl::GetRequestTime() const {
  return GetResponse().SentTime() - StartTime();
}

std::chrono::duration<double> HttpRequestImpl::GetResponseTime() const {
  return GetResponse().ReadyTime() - StartTime();
}

const std::string& HttpRequestImpl::GetHost() const {
  return GetHeader(USERVER_NAMESPACE::http::headers::kHost);
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

const FormDataArg& HttpRequestImpl::GetFormDataArg(
    const std::string& arg_name) const {
  static const FormDataArg kEmptyFormDataArg{};

  auto it = form_data_args_.find(arg_name);
  if (it == form_data_args_.end()) return kEmptyFormDataArg;
  return it->second.at(0);
}

const std::vector<FormDataArg>& HttpRequestImpl::GetFormDataArgVector(
    const std::string& arg_name) const {
  static const std::vector<FormDataArg> kEmptyFormDataArgVector{};

  auto it = form_data_args_.find(arg_name);
  if (it == form_data_args_.end()) return kEmptyFormDataArgVector;
  return it->second;
}

bool HttpRequestImpl::HasFormDataArg(const std::string& arg_name) const {
  auto it = form_data_args_.find(arg_name);
  return (it != form_data_args_.end());
}

size_t HttpRequestImpl::FormDataArgCount() const {
  return form_data_args_.size();
}

std::vector<std::string> HttpRequestImpl::FormDataArgNames() const {
  std::vector<std::string> res;
  res.reserve(form_data_args_.size());
  for (const auto& [name, _] : form_data_args_) res.push_back(name);
  return res;
}

const std::string& HttpRequestImpl::GetPathArg(
    const std::string& arg_name) const {
  auto it = path_args_by_name_index_.find(arg_name);
  if (it == path_args_by_name_index_.end()) return kEmptyString;
  UASSERT(it->second < path_args_.size());
  return path_args_[it->second];
}

const std::string& HttpRequestImpl::GetPathArg(size_t index) const {
  return index < PathArgCount() ? path_args_[index] : kEmptyString;
}

bool HttpRequestImpl::HasPathArg(const std::string& arg_name) const {
  return path_args_by_name_index_.find(arg_name) !=
         path_args_by_name_index_.end();
}

bool HttpRequestImpl::HasPathArg(size_t index) const {
  return index < PathArgCount();
}

size_t HttpRequestImpl::PathArgCount() const { return path_args_.size(); }

const std::string& HttpRequestImpl::GetHeader(
    std::string_view header_name) const {
  auto it = headers_.find(header_name);
  if (it == headers_.end()) return kEmptyString;
  return it->second;
}

const std::string& HttpRequestImpl::GetHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name)
    const {
  auto it = headers_.find(header_name);
  if (it == headers_.end()) return kEmptyString;
  return it->second;
}

bool HttpRequestImpl::HasHeader(std::string_view header_name) const {
  return headers_.count(header_name) != 0;
}

bool HttpRequestImpl::HasHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name)
    const {
  return headers_.count(header_name) != 0;
}

size_t HttpRequestImpl::HeaderCount() const { return headers_.size(); }

void HttpRequestImpl::RemoveHeader(std::string_view header_name) {
  headers_.erase(header_name);
}

void HttpRequestImpl::RemoveHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
  headers_.erase(header_name);
}

HttpRequest::HeadersMapKeys HttpRequestImpl::GetHeaderNames() const {
  return HttpRequest::HeadersMapKeys{headers_};
}

const HttpRequest::HeadersMap& HttpRequestImpl::GetHeaders() const {
  return headers_;
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

HttpRequest::CookiesMapKeys HttpRequestImpl::GetCookieNames() const {
  return HttpRequest::CookiesMapKeys{cookies_};
}

const HttpRequest::CookiesMap& HttpRequestImpl::GetCookies() const {
  return cookies_;
}

void HttpRequestImpl::SetRequestBody(std::string body) {
  request_body_ = std::move(body);
}

void HttpRequestImpl::ParseArgsFromBody() {
  USERVER_NAMESPACE::http::parser::ParseArgs(request_body_, request_args_);
}

bool HttpRequestImpl::IsBodyCompressed() const {
  const auto& encoding =
      GetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding);
  return !encoding.empty() && encoding != "identity";
}

void HttpRequestImpl::SetPathArgs(
    std::vector<std::pair<std::string, std::string>> args) {
  path_args_.clear();
  path_args_.reserve(args.size());

  path_args_by_name_index_.clear();
  for (auto& [name, value] : args) {
    path_args_.push_back(std::move(value));
    if (!name.empty()) {
      path_args_by_name_index_[std::move(name)] = path_args_.size() - 1;
    }
  }
}

void HttpRequestImpl::SetMatchedPathLength(size_t length) {
  path_suffix_ = request_path_.substr(length);
}

void HttpRequestImpl::AccountResponseTime() {
  UASSERT(request_statistics_);
  auto timing = std::chrono::duration_cast<std::chrono::milliseconds>(
      finish_send_response_time_ - start_time_);
  request_statistics_->Account(GetMethod(),
                               handlers::HttpRequestStatisticsEntry{timing});
}

void HttpRequestImpl::MarkAsInternalServerError() const {
  response_.SetStatus(http::HttpStatus::kInternalServerError);
  response_.SetData({});
  response_.ClearHeaders();
}

void HttpRequestImpl::SetHttpHandler(const handlers::HttpHandlerBase& handler) {
  handler_ = &handler;
}

const handlers::HttpHandlerBase* HttpRequestImpl::GetHttpHandler() const {
  return handler_;
}

void HttpRequestImpl::SetTaskProcessor(engine::TaskProcessor& task_processor) {
  task_processor_ = &task_processor;
}

engine::TaskProcessor* HttpRequestImpl::GetTaskProcessor() const {
  return task_processor_;
}

void HttpRequestImpl::SetHttpHandlerStatistics(
    handlers::HttpRequestStatistics& stats) {
  request_statistics_ = &stats;
}

void HttpRequestImpl::WriteAccessLogs(
    const logging::LoggerPtr& logger_access,
    const logging::LoggerPtr& logger_access_tskv,
    const std::string& remote_address) const {
  if (!logger_access && !logger_access_tskv) return;

  const auto tp = utils::datetime::WallCoarseClock::now();
  WriteAccessLog(logger_access, tp, remote_address);
  WriteAccessTskvLog(logger_access_tskv, tp, remote_address);
}

void HttpRequestImpl::WriteAccessLog(
    const logging::LoggerPtr& logger_access,
    utils::datetime::WallCoarseClock::time_point tp,
    const std::string& remote_address) const {
  if (!logger_access) return;

  logger_access->Log(
      logging::Level::kInfo,
      fmt::format(
          R"([{}] {} {} "{} {} HTTP/{}.{}" {} "{}" "{}" "{}" {:0.6f} - {} {:0.6f})",
          utils::datetime::LocalTimezoneTimestring(tp,
                                                   "%Y-%m-%d %H:%M:%E6S %Ez"),
          EscapeForAccessLog(GetHost()), EscapeForAccessLog(remote_address),
          EscapeForAccessLog(GetOrigMethodStr()), EscapeForAccessLog(GetUrl()),
          GetHttpMajor(), GetHttpMinor(),
          static_cast<int>(response_.GetStatus()),
          EscapeForAccessLog(GetHeader("Referer")),
          EscapeForAccessLog(GetHeader("User-Agent")),
          EscapeForAccessLog(GetHeader("Cookie")), GetRequestTime().count(),
          GetResponse().BytesSent(), GetResponseTime().count()));
}

void HttpRequestImpl::WriteAccessTskvLog(
    const logging::LoggerPtr& logger_access_tskv,
    utils::datetime::WallCoarseClock::time_point tp,
    const std::string& remote_address) const {
  if (!logger_access_tskv) return;

  logger_access_tskv->Log(
      logging::Level::kInfo,
      fmt::format("tskv"
                  "\t{}"
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
                  utils::datetime::LocalTimezoneTimestring(
                      tp, "timestamp=%Y-%m-%dT%H:%M:%S\ttimezone=%Ez"),
                  static_cast<int>(response_.GetStatus()), GetHttpMajor(),
                  GetHttpMinor(), EscapeForAccessTskvLog(GetOrigMethodStr()),
                  EscapeForAccessTskvLog(GetUrl()),
                  EscapeForAccessTskvLog(GetHeader("Referer")),
                  EscapeForAccessTskvLog(GetHeader("Cookie")),
                  EscapeForAccessTskvLog(GetHeader("User-Agent")),
                  EscapeForAccessTskvLog(GetHost()),
                  EscapeForAccessTskvLog(remote_address),
                  EscapeForAccessTskvLog(GetHeader("X-Forwarded-For")),
                  EscapeForAccessTskvLog(GetHeader("X-Real-IP")),
                  EscapeForAccessTskvLog(GetHeader("X-YaRequestId")),
                  EscapeForAccessTskvLog(GetHost()),
                  EscapeForAccessTskvLog(remote_address),
                  GetRequestTime().count(), GetResponseTime().count(),
                  EscapeForAccessTskvLog(RequestBody())));
}

}  // namespace server::http

USERVER_NAMESPACE_END
