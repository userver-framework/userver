#include "http_request_parser.hpp"

#include <cassert>

#include <logging/log.hpp>
#include <server/request/request_base.hpp>

namespace server {
namespace http {

const http_parser_settings HttpRequestParser::parser_settings = {
    /*.on_message_begin = */ HttpRequestParser::OnMessageBegin,
    /*.on_url = */ HttpRequestParser::OnUrl,
    /*.on_status_complete = */ nullptr,
    /*.on_header_field = */ HttpRequestParser::OnHeaderField,
    /*.on_header_value = */ HttpRequestParser::OnHeaderValue,
    /*.on_headers_complete = */ HttpRequestParser::OnHeadersComplete,
    /*.on_body = */ HttpRequestParser::OnBody,
    /*.on_message_complete = */ HttpRequestParser::OnMessageComplete};

HttpRequestParser::HttpRequestParser(
    const HttpRequestHandler& request_handler,
    const request::RequestConfig& request_config,
    OnNewRequestCb&& on_new_request_cb)
    : request_handler_(request_handler),
      on_new_request_cb_(std::move(on_new_request_cb)) {
  http_parser_init(&parser_, HTTP_REQUEST);
  parser_.data = this;

  auto max_url_size = request_config.ParseOptionalUint64("max_url_size");
  if (max_url_size) request_constructor_config_.max_url_size = *max_url_size;

  auto max_request_size =
      request_config.ParseOptionalUint64("max_request_size");
  if (max_request_size)
    request_constructor_config_.max_request_size = *max_request_size;

  auto max_headers_size =
      request_config.ParseOptionalUint64("max_headers_size");
  if (max_headers_size)
    request_constructor_config_.max_headers_size = *max_headers_size;

  auto parse_args_from_body =
      request_config.ParseOptionalBool("parse_args_from_body");
  if (parse_args_from_body)
    request_constructor_config_.parse_args_from_body = *parse_args_from_body;
}

bool HttpRequestParser::Parse(const char* data, size_t size) {
  size_t parsed = http_parser_execute(&parser_, &parser_settings, data, size);
  if (parsed != size) {
    LOG_WARNING() << "parsed=" << parsed << " size=" << size
                  << " error_description="
                  << http_errno_description(HTTP_PARSER_ERRNO(&parser_));
    FinalizeRequest();
    return false;
  }
  if (parser_.upgrade) {
    LOG_WARNING() << "upgrade detected";
    FinalizeRequest();
    return false;
  }
  return true;
}

size_t HttpRequestParser::ParsingRequestCount() const {
  return parsing_request_count_;
}

int HttpRequestParser::OnMessageBegin(http_parser* p) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnMessageBeginImpl(p);
}

int HttpRequestParser::OnHeadersComplete(http_parser* p) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnHeadersCompleteImpl(p);
}

int HttpRequestParser::OnMessageComplete(http_parser* p) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnMessageCompleteImpl(p);
}

int HttpRequestParser::OnUrl(http_parser* p, const char* data, size_t size) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnUrlImpl(p, data, size);
}

int HttpRequestParser::OnHeaderField(http_parser* p, const char* data,
                                     size_t size) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnHeaderFieldImpl(p, data, size);
}

int HttpRequestParser::OnHeaderValue(http_parser* p, const char* data,
                                     size_t size) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnHeaderValueImpl(p, data, size);
}

int HttpRequestParser::OnBody(http_parser* p, const char* data, size_t size) {
  HttpRequestParser* http_request_parser =
      static_cast<HttpRequestParser*>(p->data);
  assert(http_request_parser != nullptr);
  return http_request_parser->OnBodyImpl(p, data, size);
}

int HttpRequestParser::OnMessageBeginImpl(http_parser* p) {
  LOG_TRACE() << "message begin (method="
              << http_method_str(static_cast<http_method>(p->method)) << ')';
  CreateRequestConstructor();
  request_constructor_->SetMethod(static_cast<http_method>(p->method));
  return 0;
}

int HttpRequestParser::OnUrlImpl(http_parser*, const char* data, size_t size) {
  assert(request_constructor_);
  LOG_TRACE() << "url: '" << std::string(data, size) << '\'';
  try {
    request_constructor_->AppendUrl(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append url: " << ex.what();
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnHeaderFieldImpl(http_parser* p, const char* data,
                                         size_t size) {
  assert(request_constructor_);
  LOG_TRACE() << "header field: '" << std::string(data, size) << "'";
  if (!CheckUrlComplete(p)) return -1;
  try {
    request_constructor_->AppendHeaderField(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append header field: " << ex.what();
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnHeaderValueImpl(http_parser* p, const char* data,
                                         size_t size) {
  assert(request_constructor_);
  if (!CheckUrlComplete(p)) return -1;
  LOG_TRACE() << "header value: '" << std::string(data, size) << '\'';
  try {
    request_constructor_->AppendHeaderValue(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append header value: " << ex.what();
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnHeadersCompleteImpl(http_parser* p) {
  assert(request_constructor_);
  if (!CheckUrlComplete(p)) return -1;
  try {
    request_constructor_->AppendHeaderField("", 0);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append header value: " << ex.what();
    return -1;
  }
  LOG_TRACE() << "headers complete";
  return 0;
}

int HttpRequestParser::OnBodyImpl(http_parser* p, const char* data,
                                  size_t size) {
  assert(request_constructor_);
  if (!CheckUrlComplete(p)) return -1;
  LOG_TRACE() << "body: '" << std::string(data, size) << "'";
  try {
    request_constructor_->AppendBody(data, size);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't append body: " << ex.what();
    return -1;
  }
  return 0;
}

int HttpRequestParser::OnMessageCompleteImpl(http_parser* p) {
  assert(request_constructor_);
  if (p->upgrade) {
    LOG_WARNING() << "upgrade detected";
    return -1;  // error
  }
  if (!CheckUrlComplete(p)) return -1;
  LOG_TRACE() << "message complete";
  if (!FinalizeRequest()) return -1;
  return 0;
}

void HttpRequestParser::CreateRequestConstructor() {
  ++parsing_request_count_;
  request_constructor_ = std::make_unique<HttpRequestConstructor>(
      request_constructor_config_, request_handler_);
  url_complete_ = false;
}

bool HttpRequestParser::CheckUrlComplete(http_parser* p) {
  if (url_complete_) return true;
  url_complete_ = true;
  request_constructor_->SetHttpMajor(p->http_major);
  request_constructor_->SetHttpMinor(p->http_minor);
  try {
    request_constructor_->ParseUrl();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse url: " << ex.what();
    return false;
  }
  return true;
}

bool HttpRequestParser::FinalizeRequest() {
  bool res = FinalizeRequestImpl();
  --parsing_request_count_;
  request_constructor_ = nullptr;
  return res;
}

bool HttpRequestParser::FinalizeRequestImpl() {
  if (!request_constructor_) CreateRequestConstructor();

  if (auto request = request_constructor_->Finalize())
    on_new_request_cb_(std::move(request));
  else {
    LOG_ERROR() << "request is null after Finalize()";
    return false;
  }
  return true;
}

}  // namespace http
}  // namespace server
