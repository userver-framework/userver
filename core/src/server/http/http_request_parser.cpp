#include "http_request_parser.hpp"

#include <userver/logging/log.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

HttpMethod ConvertHttpMethod(llhttp_method method) {
  switch (method) {
    case HTTP_DELETE:
      return HttpMethod::kDelete;
    case HTTP_GET:
      return HttpMethod::kGet;
    case HTTP_HEAD:
      return HttpMethod::kHead;
    case HTTP_POST:
      return HttpMethod::kPost;
    case HTTP_PUT:
      return HttpMethod::kPut;
    case HTTP_CONNECT:
      return HttpMethod::kConnect;
    case HTTP_PATCH:
      return HttpMethod::kPatch;
    case HTTP_OPTIONS:
      return HttpMethod::kOptions;
    default:
      return HttpMethod::kUnknown;
  }
}

}  // namespace

const llhttp_settings_t HttpRequestParser::parser_settings = []() {
  llhttp_settings_t settings{};
  llhttp_settings_init(&settings);
  settings.on_message_begin = HttpRequestParser::OnMessageBegin;
  settings.on_url = HttpRequestParser::OnUrl;
  settings.on_header_field = HttpRequestParser::OnHeaderField;
  settings.on_header_value = HttpRequestParser::OnHeaderValue;
  settings.on_headers_complete = HttpRequestParser::OnHeadersComplete;
  settings.on_body = HttpRequestParser::OnBody;
  settings.on_message_complete = HttpRequestParser::OnMessageComplete;
  return settings;
}();

HttpRequestParser::HttpRequestParser(
    const HandlerInfoIndex& handler_info_index,
    const request::HttpRequestConfig& request_config,
    OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
    request::ResponseDataAccounter& data_accounter)
    : handler_info_index_(handler_info_index),
      request_constructor_config_{request_config},
      on_new_request_cb_(std::move(on_new_request_cb)),
      stats_(stats),
      data_accounter_(data_accounter) {
  llhttp_init(&parser_, HTTP_REQUEST, &parser_settings);
  parser_.data = this;
}

bool HttpRequestParser::Parse(const char* data, size_t size) {
  const auto err = llhttp_execute(&parser_, data, size);
  if (err != HPE_OK) {
    const auto parsed =
        static_cast<size_t>(llhttp_get_error_pos(&parser_) - data + 1);
    LOG_WARNING() << "parsed=" << parsed << " size=" << size
                  << " error_description=" << llhttp_errno_name(err);
    FinalizeRequest();
    return false;
  }
  if (parser_.upgrade) {
    FinalizeRequest();
    return false;
  }
  return true;
}

int HttpRequestParser::OnMessageBegin(llhttp_t* p) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnMessageBeginImpl(p);
}

int HttpRequestParser::OnHeadersComplete(llhttp_t* p) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnHeadersCompleteImpl(p);
}

int HttpRequestParser::OnMessageComplete(llhttp_t* p) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnMessageCompleteImpl(p);
}

int HttpRequestParser::OnUrl(llhttp_t* p, const char* data, size_t size) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnUrlImpl(p, data, size);
}

int HttpRequestParser::OnHeaderField(llhttp_t* p, const char* data,
                                     size_t size) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnHeaderFieldImpl(p, data, size);
}

int HttpRequestParser::OnHeaderValue(llhttp_t* p, const char* data,
                                     size_t size) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnHeaderValueImpl(p, data, size);
}

int HttpRequestParser::OnBody(llhttp_t* p, const char* data, size_t size) {
  auto* http_request_parser = static_cast<HttpRequestParser*>(p->data);
  UASSERT(http_request_parser != nullptr);
  return http_request_parser->OnBodyImpl(p, data, size);
}

int HttpRequestParser::OnMessageBeginImpl(llhttp_t*) {
  LOG_TRACE() << "message begin";
  CreateRequestConstructor();
  return 0;
}

int HttpRequestParser::OnUrlImpl(llhttp_t* p, const char* data, size_t size) {
  UASSERT(request_constructor_);
  LOG_TRACE() << "url: '" << std::string_view(data, size) << '\'';
  request_constructor_->SetMethod(
      ConvertHttpMethod(static_cast<llhttp_method>(p->method)));
  try {
    request_constructor_->AppendUrl(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append url: " << ex;
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnHeaderFieldImpl(llhttp_t* p, const char* data,
                                         size_t size) {
  UASSERT(request_constructor_);
  LOG_TRACE() << "header field: '" << std::string_view(data, size) << "'";
  if (!CheckUrlComplete(p)) return -1;
  try {
    request_constructor_->AppendHeaderField(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append header field: " << ex;
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnHeaderValueImpl(llhttp_t* p, const char* data,
                                         size_t size) {
  UASSERT(request_constructor_);
  if (!CheckUrlComplete(p)) return -1;
  LOG_TRACE() << "header value: '" << std::string_view(data, size) << '\'';
  try {
    request_constructor_->AppendHeaderValue(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append header value: " << ex;
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnHeadersCompleteImpl(llhttp_t* p) {
  UASSERT(request_constructor_);
  if (!CheckUrlComplete(p)) return -1;
  try {
    request_constructor_->AppendHeaderField("", 0);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append header value: " << ex;
    return -1;
  }
  LOG_TRACE() << "headers complete";
  return 0;
}

int HttpRequestParser::OnBodyImpl(llhttp_t* p, const char* data, size_t size) {
  UASSERT(request_constructor_);
  if (!CheckUrlComplete(p)) return -1;
  LOG_TRACE() << "body: '" << std::string_view(data, size) << "'";
  try {
    request_constructor_->AppendBody(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append body: " << ex;
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnMessageCompleteImpl(llhttp_t* p) {
  UASSERT(request_constructor_);
  if (p->upgrade) {
    return -1;  // error
  }
  request_constructor_->SetIsFinal(!llhttp_should_keep_alive(p));
  if (!CheckUrlComplete(p)) return -1;
  LOG_TRACE() << "message complete";
  if (!FinalizeRequest()) return -1;
  return 0;
}

void HttpRequestParser::CreateRequestConstructor() {
  stats_.parsing_request_count.Add(1);
  request_constructor_.emplace(request_constructor_config_, handler_info_index_,
                               data_accounter_);
  url_complete_ = false;
}

bool HttpRequestParser::CheckUrlComplete(llhttp_t* p) {
  if (url_complete_) return true;
  url_complete_ = true;
  request_constructor_->SetMethod(
      ConvertHttpMethod(static_cast<llhttp_method>(p->method)));
  request_constructor_->SetHttpMajor(p->http_major);
  request_constructor_->SetHttpMinor(p->http_minor);
  try {
    request_constructor_->ParseUrl();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse url: " << ex;
    return false;
  }
  return true;
}

bool HttpRequestParser::FinalizeRequest() {
  bool res = FinalizeRequestImpl();
  stats_.parsing_request_count.Subtract(1);
  request_constructor_.reset();
  return res;
}

bool HttpRequestParser::FinalizeRequestImpl() {
  if (!request_constructor_) CreateRequestConstructor();

  if (auto request = request_constructor_->Finalize()) {
    on_new_request_cb_(std::move(request));
  } else {
    LOG_ERROR() << "request is null after Finalize()";
    return false;
  }
  return true;
}

}  // namespace server::http

USERVER_NAMESPACE_END
