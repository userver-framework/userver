#include "http_request_constructor.hpp"

#include <algorithm>
#include <cassert>
#include <sstream>

#include <logging/log.hpp>
#include <server/http/http_status.hpp>
#include <utils/exception.hpp>

namespace server {
namespace http {

namespace {

const std::string kCookieHeader = "Cookie";

inline void Strip(const char*& begin, const char*& end) {
  while (begin < end && isspace(*begin)) ++begin;
  while (begin < end && isspace(end[-1])) --end;
}

}  // namespace

HttpRequestConstructor::HttpRequestConstructor(
    Config config, const HandlerInfoIndex& handler_info_index)
    : config_(config),
      handler_info_index_(handler_info_index),
      request_(std::make_unique<HttpRequestImpl>()) {}

void HttpRequestConstructor::SetMethod(HttpMethod method) {
  request_->orig_method_ = method;
  request_->method_ = (method == HttpMethod::kHead ? HttpMethod::kGet : method);
}

void HttpRequestConstructor::SetHttpMajor(unsigned short http_major) {
  request_->http_major_ = http_major;
}

void HttpRequestConstructor::SetHttpMinor(unsigned short http_minor) {
  request_->http_minor_ = http_minor;
}

void HttpRequestConstructor::AppendUrl(const char* data, size_t size) {
  // using common limits in checks
  AccountUrlSize(size);
  AccountRequestSize(size);

  request_->url_.append(data, size);
}

void HttpRequestConstructor::ParseUrl() {
  LOG_TRACE() << "parse path from '" << request_->url_ << '\'';
  if (http_parser_parse_url(request_->url_.data(), request_->url_.size(),
                            request_->method_ == HttpMethod::kConnect,
                            &parsed_url_)) {
    SetStatus(Status::kParseUrlError);
    throw std::runtime_error("error in http_parser_parse_url()");
  }

  if (parsed_url_.field_set & (1 << http_parser_url_fields::UF_PATH)) {
    const auto& str_info =
        parsed_url_.field_data[http_parser_url_fields::UF_PATH];
    request_->request_path_ = request_->url_.substr(str_info.off, str_info.len);
    LOG_TRACE() << "path='" << request_->request_path_ << '\'';
  } else {
    SetStatus(Status::kParseUrlError);
    throw std::runtime_error("can't parse path");
  }

  auto handler_info = handler_info_index_.GetHandlerInfo(
      request_->GetMethod(), request_->GetRequestPath());
  request_->SetMatchedPathLength(handler_info.matched_path_length);

  if (handler_info.handler) {
    const auto& handler_config = handler_info.handler->GetConfig();
    if (handler_config.max_url_size)
      config_.max_url_size = *handler_config.max_url_size;
    if (handler_config.max_request_size)
      config_.max_request_size = *handler_config.max_request_size;
    if (handler_config.max_headers_size)
      config_.max_headers_size = *handler_config.max_headers_size;
    if (handler_config.parse_args_from_body)
      config_.parse_args_from_body = *handler_config.parse_args_from_body;
  } else {
    SetStatus(Status::kHandlerNotFound);
  }

  // recheck sizes using per-handler limits
  AccountUrlSize(0);
  AccountRequestSize(0);

  url_parsed_ = true;
}

void HttpRequestConstructor::AppendHeaderField(const char* data, size_t size) {
  if (header_value_flag_) {
    AddHeader();
    header_value_flag_ = false;
  }
  header_field_flag_ = true;

  AccountHeadersSize(size);
  AccountRequestSize(size);

  header_field_.append(data, size);
}

void HttpRequestConstructor::AppendHeaderValue(const char* data, size_t size) {
  assert(header_field_flag_);
  header_value_flag_ = true;

  AccountHeadersSize(size);
  AccountRequestSize(size);

  header_value_.append(data, size);
}

void HttpRequestConstructor::AppendBody(const char* data, size_t size) {
  AccountRequestSize(size);
  request_->request_body_.append(data, size);
}

void HttpRequestConstructor::SetIsFinal(bool is_final) {
  request_->is_final_ = is_final;
}

namespace {

template <typename Array, typename DumpElem>
std::ostringstream& DumpArray(std::ostringstream& os, const std::string& name,
                              const Array& array, DumpElem dump_elem) {
  bool first = true;
  os << name << ":[";
  for (const auto& elem : array) {
    if (first)
      first = false;
    else
      os << ", ";
    dump_elem(elem);
  }
  os << ']';
  return os;
}

}  // namespace

std::shared_ptr<request::RequestBase> HttpRequestConstructor::Finalize() {
  LOG_TRACE() << "method=" << request_->GetMethodStr()
              << " orig_method=" << request_->GetOrigMethodStr();

  FinalizeImpl();

  CheckStatus();

  return std::move(request_);
}

void HttpRequestConstructor::FinalizeImpl() {
  if (status_ != Status::kOk) return;

  if (!url_parsed_) {
    SetStatus(Status::kBadRequest);
    return;
  }

  try {
    ParseArgs(parsed_url_);
    if (config_.parse_args_from_body)
      ParseArgs(request_->request_body_.data(), request_->request_body_.size());
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse args: " << ex;
    SetStatus(Status::kParseArgsError);
    return;
  }

  assert(std::all_of(request_->request_args_.begin(),
                     request_->request_args_.end(),
                     [](const auto& arg) { return !arg.second.empty(); }));

  LOG_TRACE() << DumpRequestArgs();
  LOG_TRACE() << DumpHeaders();

  try {
    ParseCookies();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse cookies: " << ex;
    SetStatus(Status::kParseCookiesError);
    return;
  }

  LOG_TRACE() << DumpCookies();
}

std::string HttpRequestConstructor::UrlDecode(const char* data,
                                              const char* data_end) {
  // Fast path: no %, just id
  if (!memchr(data, '%', data_end - data)) {
    return std::string(data, data_end);
  }

  std::string res;
  for (const char* ptr = data; ptr < data_end; ++ptr) {
    if (*ptr == '%') {
      if (ptr + 2 < data_end && isxdigit(ptr[1]) && isxdigit(ptr[2])) {
        unsigned int num;
        sscanf(ptr + 1, "%2x", &num);
        res += static_cast<char>(num);
        ptr += 2;
      } else {
        throw std::runtime_error("invalid percent-encoding sequence");
      }
    } else {
      res += *ptr;
    }
  }
  return res;
}

void HttpRequestConstructor::ParseArgs(const http_parser_url& url) {
  if (url.field_set & (1 << http_parser_url_fields::UF_QUERY)) {
    const auto& str_info = url.field_data[http_parser_url_fields::UF_QUERY];
    ParseArgs(request_->url_.data() + str_info.off, str_info.len);
    LOG_TRACE() << "query="
                << request_->url_.substr(str_info.off, str_info.len);
  }
}

void HttpRequestConstructor::ParseArgs(const char* data, size_t size) {
  const char* end = data + size;
  const char* key_begin = data;
  const char* key_end = data;
  bool parse_key = true;
  for (const char* ptr = data; ptr <= end; ++ptr) {
    if (ptr == end || *ptr == '&') {
      if (!parse_key) {
        const char* value_begin = key_end + 1;
        const char* value_end = ptr;
        if (key_begin < key_end && value_begin < value_end) {
          std::vector<std::string>& arg_values =
              request_->request_args_[UrlDecode(key_begin, key_end)];
          arg_values.emplace_back(UrlDecode(value_begin, value_end));
        }
      }
      parse_key = true;
      key_begin = ptr + 1;
      continue;
    }
    if (*ptr == '=' && parse_key) {
      parse_key = false;
      key_end = ptr;
      continue;
    }
  }
}

void HttpRequestConstructor::AddHeader() {
  assert(header_field_flag_);
  auto it = request_->headers_.find(header_field_);
  if (it == request_->headers_.end()) {
    request_->headers_.emplace(std::move(header_field_),
                               std::move(header_value_));
  } else {
    it->second += ',';
    it->second += header_value_;
  }
  header_field_.clear();
  header_value_.clear();
}

void HttpRequestConstructor::ParseCookies() {
  const std::string& cookie = request_->GetHeader(kCookieHeader);

  const char* data = cookie.data();
  size_t size = cookie.size();
  const char* end = data + size;
  const char* key_begin = data;
  const char* key_end = data;
  bool parse_key = true;
  for (const char* ptr = data; ptr <= end; ++ptr) {
    if (ptr == end || *ptr == ';') {
      const char* value_begin;
      const char* value_end;
      if (parse_key) {
        key_end = ptr;
        value_begin = value_end = ptr;
      } else {
        value_begin = key_end + 1;
        value_end = ptr;
        Strip(value_begin, value_end);
        if (value_begin + 2 <= value_end && *value_begin == '"' &&
            value_end[-1] == '"') {
          ++value_begin;
          --value_end;
        }
      }
      Strip(key_begin, key_end);
      if (key_begin < key_end)
        request_->cookies_.emplace(UrlDecode(key_begin, key_end),
                                   UrlDecode(value_begin, value_end));
      parse_key = true;
      key_begin = ptr + 1;
      continue;
    }
    if (*ptr == '=' && parse_key) {
      parse_key = false;
      key_end = ptr;
      continue;
    }
  }
}

void HttpRequestConstructor::SetStatus(HttpRequestConstructor::Status status) {
  status_ = status;
}

void HttpRequestConstructor::AccountRequestSize(size_t size) {
  request_size_ += size;
  if (request_size_ > config_.max_request_size) {
    SetStatus(Status::kRequestTooLarge);
    utils::LogErrorAndThrow(
        "request is too large, " + std::to_string(request_size_) + ">" +
        std::to_string(config_.max_request_size) +
        " (enforced by 'max_request_size' handler limit in config.yaml)");
  }
}

void HttpRequestConstructor::AccountUrlSize(size_t size) {
  url_size_ += size;
  if (url_size_ > config_.max_url_size) {
    SetStatus(Status::kUriTooLong);
    utils::LogErrorAndThrow("url is too long " + std::to_string(url_size_) +
                            ">" + std::to_string(config_.max_url_size) +
                            " (enforced by 'max_url_size' handler limit in "
                            "config.yaml)");
  }
}

void HttpRequestConstructor::AccountHeadersSize(size_t size) {
  headers_size_ += size;
  if (headers_size_ > config_.max_headers_size) {
    SetStatus(Status::kHeadersTooLarge);
    utils::LogErrorAndThrow("headers too large " +
                            std::to_string(headers_size_) + ">" +
                            std::to_string(config_.max_headers_size) +
                            " (enforced by 'max_headers_size' handler limit in "
                            "config.yaml)");
  }
}

std::string HttpRequestConstructor::DumpRequestArgs() const {
  std::ostringstream os;
  DumpArray(os, "request_args", request_->request_args_,
            [&os](const std::pair<std::string, std::vector<std::string>>& arg) {
              os << "{\"arg_name\":\"" << arg.first << "\", ";
              DumpArray(os, "\"arg_values\"", arg.second,
                        [&os](const std::string& value) {
                          os << "\"" << value << "\"";
                        });
            });
  return os.str();
}

std::string HttpRequestConstructor::DumpHeaders() const {
  std::ostringstream os;
  DumpArray(os, "headers", request_->headers_,
            [&os](const std::pair<std::string, std::string>& header) {
              os << "{\"header_name\":\"" << header.first
                 << "\", \"header_value\":\"" << header.second << "\"}";
            });
  return os.str();
}

std::string HttpRequestConstructor::DumpCookies() const {
  std::ostringstream os;
  DumpArray(os, "cookies", request_->cookies_,
            [&os](const std::pair<std::string, std::string>& cookie) {
              os << "{\"cookie_name\":\"" << cookie.first
                 << "\", \"cookie_value\":\"" << cookie.second << "\"}";
            });
  return os.str();
}

void HttpRequestConstructor::CheckStatus() const {
  switch (status_) {
    case Status::kOk:
      request_->SetResponseStatus(HttpStatus::kOk);
      break;
    case Status::kBadRequest:
      request_->SetResponseStatus(HttpStatus::kBadRequest);
      request_->GetHttpResponse().SetData("bad request");
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kUriTooLong:
      request_->SetResponseStatus(HttpStatus::kUriTooLong);
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kParseUrlError:
      request_->SetResponseStatus(HttpStatus::kBadRequest);
      request_->GetHttpResponse().SetData("invalid url");
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kHandlerNotFound:
      request_->SetResponseStatus(HttpStatus::kNotFound);
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kHeadersTooLarge:
      request_->SetResponseStatus(HttpStatus::kRequestHeaderFieldsTooLarge);
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kRequestTooLarge:
      request_->SetResponseStatus(HttpStatus::kPayloadTooLarge);
      request_->GetHttpResponse().SetData("too large request");
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kParseArgsError:
      request_->SetResponseStatus(HttpStatus::kBadRequest);
      request_->GetHttpResponse().SetData("invalid args");
      request_->GetHttpResponse().SetReady();
      break;
    case Status::kParseCookiesError:
      request_->SetResponseStatus(HttpStatus::kBadRequest);
      request_->GetHttpResponse().SetData("invalid cookies");
      request_->GetHttpResponse().SetReady();
      break;
  }
}

}  // namespace http
}  // namespace server
