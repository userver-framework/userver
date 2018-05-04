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

HttpRequestParser::HttpRequestParser(OnNewRequestCb&& on_new_request_cb)
    : on_new_request_cb_(std::move(on_new_request_cb)) {
  http_parser_init(&parser_, HTTP_REQUEST);
  parser_.data = this;
}

HttpRequestParser::~HttpRequestParser() {}

bool HttpRequestParser::Parse(const char* data, size_t size) {
  size_t parsed = http_parser_execute(&parser_, &parser_settings, data, size);
  if (parsed != size) {
    LOG_WARNING() << "parsed=" << parsed << " size=" << size
                  << " error_description="
                  << http_errno_description(HTTP_PARSER_ERRNO(&parser_));
    return false;
  }
  if (parser_.upgrade) {
    LOG_WARNING() << "upgrade detected";
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
              << http_method_str(static_cast<http_method>(p->method)) << ")";
  ++parsing_request_count_;
  request_constructor_ = std::make_unique<HttpRequestConstructor>();
  request_constructor_->SetMethod(static_cast<http_method>(p->method));
  request_constructor_->SetParseArgsFromBody(true);
  request_size_ = 0;
  return 0;
}

int HttpRequestParser::OnUrlImpl(http_parser*, const char* data, size_t size) {
  assert(request_constructor_);
  if (!UpdateRequestSize(size)) return -1;  // error
  LOG_TRACE() << "url: '" << std::string(data, size) << "'";
  request_constructor_->AppendUrl(data, size);
  return 0;
}

int HttpRequestParser::OnHeaderFieldImpl(http_parser*, const char* data,
                                         size_t size) {
  assert(request_constructor_);
  if (!UpdateRequestSize(size)) return -1;  // error
  LOG_TRACE() << "header field: '" << std::string(data, size) << "'";
  request_constructor_->AppendHeaderField(data, size);
  return 0;
}

int HttpRequestParser::OnHeaderValueImpl(http_parser*, const char* data,
                                         size_t size) {
  assert(request_constructor_);
  if (!UpdateRequestSize(size)) return -1;  // error
  LOG_TRACE() << "header value: '" << std::string(data, size) << "'";
  request_constructor_->AppendHeaderValue(data, size);
  return 0;
}

int HttpRequestParser::OnHeadersCompleteImpl(http_parser*) {
  assert(request_constructor_);
  request_constructor_->AppendHeaderField("", 0);
  LOG_TRACE() << "headers complete";
  return 0;
}

int HttpRequestParser::OnBodyImpl(http_parser*, const char* data, size_t size) {
  assert(request_constructor_);
  if (!UpdateRequestSize(size)) return -1;  // error
  LOG_TRACE() << "body: '" << std::string(data, size) << "'";
  request_constructor_->AppendBody(data, size);
  return 0;
}

int HttpRequestParser::OnMessageCompleteImpl(http_parser* p) {
  assert(request_constructor_);
  if (p->upgrade) {
    LOG_WARNING() << "upgrade detected";
    return -1;  // error
  }
  request_constructor_->SetHttpMajor(p->http_major);
  request_constructor_->SetHttpMinor(p->http_minor);
  LOG_TRACE() << "message complete";
  if (auto request = request_constructor_->Finalize())
    on_new_request_cb_(std::move(request));
  else {
    request_constructor_ = nullptr;
    --parsing_request_count_;
    return -1;  // error
  }
  --parsing_request_count_;

  request_constructor_ = nullptr;
  return 0;
}

bool HttpRequestParser::UpdateRequestSize(size_t size) {
  request_size_ += size;
  return request_size_ <= max_request_size_;
}

}  // namespace http
}  // namespace server
