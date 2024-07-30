#include <userver/utest/http_server_mock.hpp>

#include <llhttp.h>
#include <boost/algorithm/string/split.hpp>

#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace {

clients::http::HttpMethod ConvertHttpMethod(llhttp_method method) {
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

}  // namespace

class HttpConnection final {
 public:
  explicit HttpConnection(HttpServerMock& owner);

  HttpConnection(const HttpConnection& other);

  SimpleServer::Response operator()(const SimpleServer::Request& request);

 private:
  static int OnUrl(llhttp_t* p, const char* data, size_t size);
  static int OnHeaderField(llhttp_t* p, const char* data, size_t size);
  static int OnHeaderValue(llhttp_t* p, const char* data, size_t size);
  static int OnHeadersComplete(llhttp_t* p);
  static int OnBody(llhttp_t* p, const char* data, size_t size);
  static int OnMessageComplete(llhttp_t* p);

  static HttpConnection& GetConnection(llhttp_t* p);

  int DoOnUrl(llhttp_t* p, const char* data, size_t size);
  int DoOnHeaderField(const char* data, size_t size);
  int DoOnHeaderValue(const char* data, size_t size);
  int DoOnHeadersComplete();
  int DoOnBody(const char* data, size_t size);
  int DoOnMessageComplete();

  void Reset();

  static llhttp_settings_t MakeHttpParserSettings();

  static llhttp_settings_t kParserSettings;

  HttpServerMock& owner_;
  llhttp_t parser_{};

  HttpServerMock::HttpRequest http_request_;
  std::optional<HttpServerMock::HttpResponse> http_response_;

  std::string header_name_;
  std::string header_value_;
  bool reading_header_name_{true};
};

int HttpConnection::OnUrl(llhttp_t* p, const char* data, size_t size) {
  return GetConnection(p).DoOnUrl(p, data, size);
}

int HttpConnection::OnHeaderField(llhttp_t* p, const char* data, size_t size) {
  return GetConnection(p).DoOnHeaderField(data, size);
}

int HttpConnection::OnHeaderValue(llhttp_t* p, const char* data, size_t size) {
  return GetConnection(p).DoOnHeaderValue(data, size);
}

int HttpConnection::OnHeadersComplete(llhttp_t* p) {
  return GetConnection(p).DoOnHeadersComplete();
}

int HttpConnection::OnBody(llhttp_t* p, const char* data, size_t size) {
  return GetConnection(p).DoOnBody(data, size);
}

int HttpConnection::OnMessageComplete(llhttp_t* p) {
  return GetConnection(p).DoOnMessageComplete();
}

HttpConnection& HttpConnection::GetConnection(llhttp_t* p) {
  return *static_cast<HttpConnection*>(p->data);
}

int HttpConnection::DoOnUrl(llhttp_t* p, const char* data, size_t size) {
  http_request_.method =
      ConvertHttpMethod(static_cast<llhttp_method>(p->method));
  http_request_.path.append(data, size);
  return 0;
}

int HttpConnection::DoOnHeaderField(const char* data, size_t size) {
  if (!reading_header_name_) {
    http_request_.headers[std::move(header_name_)] = std::move(header_value_);
    header_name_.clear();
    header_value_.clear();
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
    http_response_ = owner_.http_handler_(http_request_);
  } catch (const std::exception& e) {
    ADD_FAILURE() << e.what();
    http_response_ = HttpServerMock::HttpResponse{500, {}, ""};
  }
  return 0;
}

llhttp_settings_t HttpConnection::MakeHttpParserSettings() {
  llhttp_settings_t settings{};
  llhttp_settings_init(&settings);

  settings.on_url = HttpConnection::OnUrl;
  settings.on_header_field = HttpConnection::OnHeaderField;
  settings.on_header_value = HttpConnection::OnHeaderValue;
  settings.on_headers_complete = HttpConnection::OnHeadersComplete;
  settings.on_body = HttpConnection::OnBody;
  settings.on_message_complete = HttpConnection::OnMessageComplete;

  return settings;
}

llhttp_settings_t HttpConnection::kParserSettings =
    HttpConnection::MakeHttpParserSettings();

HttpConnection::HttpConnection(HttpServerMock& owner) : owner_(owner) {
  llhttp_init(&parser_, HTTP_REQUEST, &kParserSettings);
  parser_.data = this;
}

HttpConnection::HttpConnection(const HttpConnection& other)
    : HttpConnection(other.owner_) {}

SimpleServer::Response HttpConnection::operator()(
    const SimpleServer::Request& request) {
  auto size = request.size();
  const auto err = llhttp_execute(&parser_, request.data(), size);
  if (err != HPE_OK) {
    const auto parsed = static_cast<size_t>(llhttp_get_error_pos(&parser_) -
                                            request.data() + 1);
    ADD_FAILURE() << "parsed=" << parsed << " size=" << size
                  << " error_description=" << llhttp_errno_name(err);
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

    Reset();
    return {std::move(data), SimpleServer::Response::kWriteAndContinue};
  } else {
    return {"", SimpleServer::Response::kTryReadMore};
  }
}

void HttpConnection::Reset() {
  llhttp_init(&parser_, HTTP_REQUEST, &kParserSettings);
  parser_.data = this;

  http_request_ = {};
  http_response_ = {};
}

HttpServerMock::HttpServerMock(HttpHandler http_handler,
                               SimpleServer::Protocol protocol)
    : http_handler_(std::move(http_handler)),
      server_(HttpConnection(*this), protocol) {}

std::string HttpServerMock::GetBaseUrl() const { return server_.GetBaseUrl(); }

std::uint64_t HttpServerMock::GetConnectionsOpenedCount() const {
  return server_.GetConnectionsOpenedCount();
}

}  // namespace utest

USERVER_NAMESPACE_END
