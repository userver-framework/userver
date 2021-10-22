#include <userver/utest/http_server_mock.hpp>

#include <http_parser.h>
#include <boost/algorithm/string/split.hpp>

#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

HttpServerMock::HttpServerMock(HttpHandler http_handler,
                               SimpleServer::Protocol protocol)
    : http_handler_(std::move(http_handler)),
      server_(OnNewConnection(), protocol) {}

std::string HttpServerMock::GetBaseUrl() const { return server_.GetBaseUrl(); }

namespace {
class HttpConnection {
 public:
  explicit HttpConnection(HttpServerMock::HttpHandler http_handler);

  HttpConnection(const HttpConnection& other);

  SimpleServer::Response operator()(const SimpleServer::Request& request);

  static int OnUrl(http_parser* p, const char* data, size_t size);
  static int OnHeaderField(http_parser* p, const char* data, size_t size);
  static int OnHeaderValue(http_parser* p, const char* data, size_t size);
  static int OnHeadersComplete(http_parser* p);
  static int OnBody(http_parser* p, const char* data, size_t size);
  static int OnMessageComplete(http_parser* p);

  static HttpConnection& GetConnection(http_parser* p);

  int DoOnUrl(http_parser* p, const char* data, size_t size);
  int DoOnHeaderField(const char* data, size_t size);
  int DoOnHeaderValue(const char* data, size_t size);
  int DoOnHeadersComplete();
  int DoOnBody(const char* data, size_t size);
  int DoOnMessageComplete();

 private:
  HttpServerMock::HttpHandler http_handler_;
  http_parser parser_{};

  HttpServerMock::HttpRequest http_request_;
  std::optional<HttpServerMock::HttpResponse> http_response_;

  std::string header_name_, header_value_;
  bool reading_header_name_{true};
};

int HttpConnection::OnUrl(http_parser* p, const char* data, size_t size) {
  return GetConnection(p).DoOnUrl(p, data, size);
}

int HttpConnection::OnHeaderField(http_parser* p, const char* data,
                                  size_t size) {
  return GetConnection(p).DoOnHeaderField(data, size);
}

int HttpConnection::OnHeaderValue(http_parser* p, const char* data,
                                  size_t size) {
  return GetConnection(p).DoOnHeaderValue(data, size);
}

int HttpConnection::OnHeadersComplete(http_parser* p) {
  return GetConnection(p).DoOnHeadersComplete();
}

int HttpConnection::OnBody(http_parser* p, const char* data, size_t size) {
  return GetConnection(p).DoOnBody(data, size);
}

int HttpConnection::OnMessageComplete(http_parser* p) {
  return GetConnection(p).DoOnMessageComplete();
}

HttpConnection& HttpConnection::GetConnection(http_parser* p) {
  return *static_cast<HttpConnection*>(p->data);
}

clients::http::HttpMethod ConvertHttpMethod(http_method method) {
  switch (method) {
    case HTTP_DELETE:
      return clients::http::HttpMethod::kDelete;
    case HTTP_GET:
      return clients::http::HttpMethod::kGet;
    case HTTP_HEAD:
      return clients::http::HttpMethod::kHead;
    case HTTP_POST:
      return clients::http::HttpMethod::kPost;
    case HTTP_PUT:
      return clients::http::HttpMethod::kPut;
    case HTTP_PATCH:
      return clients::http::HttpMethod::kPatch;
    case HTTP_OPTIONS:
      return clients::http::HttpMethod::kOptions;
    default:
      ADD_FAILURE() << "Unknown HTTP method " << method;
      return clients::http::HttpMethod::kGet;
  }
}

int HttpConnection::DoOnUrl(http_parser* p, const char* data, size_t size) {
  http_request_.method = ConvertHttpMethod(static_cast<http_method>(p->method));
  http_request_.path += std::string(data, size);
  return 0;
}

int HttpConnection::DoOnHeaderField(const char* data, size_t size) {
  if (!reading_header_name_) {
    http_request_.headers[std::move(header_name_)] = std::move(header_value_);
    reading_header_name_ = true;
  }

  header_name_ += std::string(data, size);
  return 0;
}

int HttpConnection::DoOnHeaderValue(const char* data, size_t size) {
  reading_header_name_ = false;
  header_value_ += std::string(data, size);

  return 0;
}

int HttpConnection::DoOnHeadersComplete() {
  http_request_.headers[std::move(header_name_)] = std::move(header_value_);
  return 0;
}

int HttpConnection::DoOnBody(const char* data, size_t size) {
  http_request_.body = std::string(data, size);
  return 0;
}

int HttpConnection::DoOnMessageComplete() {
  auto qm_pos = http_request_.path.find('?');
  if (qm_pos != std::string::npos) {
    auto query = http_request_.path.substr(qm_pos + 1);
    http_request_.path.resize(qm_pos);

    if (!query.empty()) {
      std::vector<std::string> query_values;
      boost::split(query_values, query, [](char c) { return c == '&'; });
      for (const auto& value : query_values) {
        auto eq_pos = value.find('=');
        EXPECT_NE(std::string::npos, eq_pos) << "Bad query: " << query;
        if (eq_pos != std::string::npos)
          http_request_.query[value.substr(0, eq_pos)] =
              value.substr(eq_pos + 1);
      }
    }
  }

  try {
    http_response_ = http_handler_(http_request_);
  } catch (const std::exception& e) {
    ADD_FAILURE() << e.what();
    http_response_ = HttpServerMock::HttpResponse{500, {}, ""};
  }
  return 0;
}

http_parser_settings InitHttpParserSettings() {
  http_parser_settings settings{};

  settings.on_url = HttpConnection::OnUrl;
  settings.on_header_field = HttpConnection::OnHeaderField;
  settings.on_header_value = HttpConnection::OnHeaderValue;
  settings.on_headers_complete = HttpConnection::OnHeadersComplete;
  settings.on_body = HttpConnection::OnBody;
  settings.on_message_complete = HttpConnection::OnMessageComplete;

  return settings;
}

const http_parser_settings parser_settings = InitHttpParserSettings();

HttpConnection::HttpConnection(HttpServerMock::HttpHandler http_handler)
    : http_handler_(std::move(http_handler)) {
  http_parser_init(&parser_, HTTP_REQUEST);
  parser_.data = this;
}

HttpConnection::HttpConnection(const HttpConnection& other)
    : HttpConnection(other.http_handler_) {}

SimpleServer::Response HttpConnection::operator()(
    const SimpleServer::Request& request) {
  auto size = request.size();
  size_t parsed =
      http_parser_execute(&parser_, &parser_settings, request.data(), size);

  if (parsed != size) {
    ADD_FAILURE() << "parsed=" << parsed << " size=" << size
                  << " error_description="
                  << http_errno_description(HTTP_PARSER_ERRNO(&parser_));
    return {"", SimpleServer::Response::kWriteAndClose};
  }
  if (parser_.upgrade) {
    ADD_FAILURE() << "Upgrade requested";
    return {"", SimpleServer::Response::kWriteAndClose};
  }

  if (http_response_) {
    auto data = "HTTP/1.1 " + std::to_string(http_response_->response_status) +
                " CODE\r\n";
    for (const auto& it : http_response_->headers) {
      data += it.first + ": " + it.second + "\r\n";
    }
    data += "Content-Length: " + std::to_string(http_response_->body.size());
    data += "\r\n\r\n" + http_response_->body;  // TODO

    return {data, SimpleServer::Response::kWriteAndClose};
  } else {
    return {"", SimpleServer::Response::kTryReadMore};
  }
}

}  // namespace

SimpleServer::OnRequest HttpServerMock::OnNewConnection() {
  return HttpConnection(http_handler_);
}
}  // namespace utest

USERVER_NAMESPACE_END
