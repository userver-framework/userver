#include "http_request_constructor.hpp"

#include <algorithm>

#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/exception.hpp>

#include "multipart_form_data_parser.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

inline void Strip(const char*& begin, const char*& end) {
  while (begin < end && isspace(*begin)) ++begin;
  while (begin < end && isspace(end[-1])) --end;
}

void StripDuplicateStartingSlashes(std::string& s) {
  if (s.empty() || s[0] != '/') return;

  size_t non_slash_pos = s.find_first_not_of('/');
  if (non_slash_pos == std::string::npos) {
    // all symbols are slashes
    non_slash_pos = s.size();
  }

  if (non_slash_pos == 1) {
    // fast path, no strip
    return;
  }

  s = s.substr(non_slash_pos - 1);
}

}  // namespace

HttpRequestConstructor::HttpRequestConstructor(
    Config config, const HandlerInfoIndex& handler_info_index,
    request::ResponseDataAccounter& data_accounter)
    : config_(config),
      handler_info_index_(handler_info_index),
      request_(std::make_shared<HttpRequestImpl>(data_accounter)) {}

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
    throw std::runtime_error("error in http_parser_parse_url() for url '" +
                             request_->url_ + '\'');
  }

  if (parsed_url_.field_set & (1 << http_parser_url_fields::UF_PATH)) {
    const auto& str_info =
        parsed_url_.field_data[http_parser_url_fields::UF_PATH];

    request_->request_path_ = request_->url_.substr(str_info.off, str_info.len);
    StripDuplicateStartingSlashes(request_->request_path_);
    LOG_TRACE() << "path='" << request_->request_path_ << '\'';
  } else {
    SetStatus(Status::kParseUrlError);
    throw std::runtime_error("no path in url '" + request_->url_ + '\'');
  }

  auto match_result = handler_info_index_.MatchRequest(
      request_->GetMethod(), request_->GetRequestPath());
  const auto* handler_info = match_result.handler_info;

  request_->SetMatchedPathLength(match_result.matched_path_length);
  request_->SetPathArgs(std::move(match_result.args_from_path));

  if (!handler_info && request_->GetMethod() == HttpMethod::kOptions &&
      match_result.status == MatchRequestResult::Status::kMethodNotAllowed) {
    handler_info = handler_info_index_.GetFallbackHandler(
        handlers::FallbackHandler::kImplicitOptions);
    if (handler_info) match_result.status = MatchRequestResult::Status::kOk;
  }

  if (handler_info) {
    UASSERT(match_result.status == MatchRequestResult::Status::kOk);
    const auto& handler_config = handler_info->handler.GetConfig();
    config_.max_request_size = handler_config.request_config.max_request_size;
    config_.max_headers_size = handler_config.request_config.max_headers_size;
    config_.parse_args_from_body =
        handler_config.request_config.parse_args_from_body;
    if (handler_config.decompress_request) config_.decompress_request = true;

    request_->SetTaskProcessor(handler_info->task_processor);
    request_->SetHttpHandler(handler_info->handler);
    request_->SetHttpHandlerStatistics(
        handler_info->handler.GetRequestStatistics());
  } else {
    if (match_result.status == MatchRequestResult::Status::kMethodNotAllowed) {
      SetStatus(Status::kMethodNotAllowed);
    } else {
      SetStatus(Status::kHandlerNotFound);
      LOG_WARNING() << "URL \"" << request_->GetUrl() << "\" not found";
    }
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
  UASSERT(header_field_flag_);
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

std::shared_ptr<request::RequestBase> HttpRequestConstructor::Finalize() {
  LOG_TRACE() << "method=" << request_->GetMethodStr()
              << " orig_method=" << request_->GetOrigMethodStr();

  FinalizeImpl();

  CheckStatus();

  return std::move(request_);  // request_ is left empty
}

void HttpRequestConstructor::FinalizeImpl() {
  if (status_ != Status::kOk &&
      (!config_.testing_mode || status_ != Status::kHandlerNotFound)) {
    return;
  }

  if (!url_parsed_) {
    SetStatus(Status::kBadRequest);
    return;
  }

  try {
    ParseArgs(parsed_url_);
    if (config_.parse_args_from_body) {
      if (!config_.decompress_request || !request_->IsBodyCompressed())
        ParseArgs(request_->request_body_.data(),
                  request_->request_body_.size());
    }
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse args: " << ex;
    SetStatus(Status::kParseArgsError);
    return;
  }

  UASSERT(std::all_of(request_->request_args_.begin(),
                      request_->request_args_.end(),
                      [](const auto& arg) { return !arg.second.empty(); }));

  LOG_TRACE() << "request_args:" << request_->request_args_;
  LOG_TRACE() << "headers:" << request_->headers_;

  try {
    ParseCookies();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse cookies: " << ex;
    SetStatus(Status::kParseCookiesError);
    return;
  }

  LOG_TRACE() << "cookies:" << request_->cookies_;

  const auto& content_type =
      request_->GetHeader(USERVER_NAMESPACE::http::headers::kContentType);
  if (IsMultipartFormDataContentType(content_type)) {
    if (!ParseMultipartFormData(content_type, request_->RequestBody(),
                                request_->form_data_args_)) {
      SetStatus(Status::kParseMultipartFormDataError);
    }
  }
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
  return USERVER_NAMESPACE::http::parser::ParseArgs(
      std::string_view(data, size), request_->request_args_);
}

void HttpRequestConstructor::AddHeader() {
  UASSERT(header_field_flag_);

  try {
    request_->headers_.InsertOrAppend(std::move(header_field_),
                                      std::move(header_value_));
  } catch (const USERVER_NAMESPACE::http::headers::HeaderMap::
               TooManyHeadersException&) {
    SetStatus(Status::kHeadersTooLarge);
    utils::LogErrorAndThrow(fmt::format(
        "HeaderMap reached its maximum capacity, already contains {} headers",
        request_->headers_.size()));
  }
  header_field_.clear();
  header_value_.clear();
}

void HttpRequestConstructor::ParseCookies() {
  const std::string& cookie =
      request_->GetHeader(USERVER_NAMESPACE::http::headers::kCookie);
  const char* data = cookie.data();
  size_t size = cookie.size();
  const char* end = data + size;
  const char* key_begin = data;
  const char* key_end = data;
  bool parse_key = true;
  for (const char* ptr = data; ptr <= end; ++ptr) {
    if (ptr == end || *ptr == ';') {
      const char* value_begin = nullptr;
      const char* value_end = nullptr;
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
      if (key_begin < key_end) {
        request_->cookies_.emplace(std::piecewise_construct,
                                   std::tie(key_begin, key_end),
                                   std::tie(value_begin, value_end));
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
        " (enforced by 'max_request_size' handler limit in config.yaml)" +
        ", url: " + (url_parsed_ ? request_->GetUrl() : "not parsed yet") +
        ", added size " + std::to_string(size));
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
                            "config.yaml)" +
                            ", url: " + request_->GetUrl() + ", added size " +
                            std::to_string(size));
  }
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
    case Status::kMethodNotAllowed:
      request_->SetResponseStatus(HttpStatus::kMethodNotAllowed);
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
    case Status::kParseMultipartFormDataError:
      request_->SetResponseStatus(HttpStatus::kBadRequest);
      request_->GetHttpResponse().SetData(
          "invalid body of multipart/form-data request");
      request_->GetHttpResponse().SetReady();
      break;
  }
}

}  // namespace server::http

USERVER_NAMESPACE_END
