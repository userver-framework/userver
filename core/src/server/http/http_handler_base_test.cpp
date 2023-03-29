#include <server/http/create_parser_test.hpp>
#include <server/http/http_handler_base.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/net/connection.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

inline server::http::HTTPHandlerBase CreateTestHttpHandlerBase(
    server::http::HttpRequestParser::OnNewRequestCb&& cb) {
  static const server::http::HandlerInfoIndex kTestHandlerInfoIndex;
  static constexpr server::request::HttpRequestConfig kTestRequestConfig{
      /*.max_url_size = */ 8192,
      /*.max_request_size = */ 1024 * 1024,
      /*.max_headers_size = */ 65536,
      /*.parse_args_from_body = */ false,
      /*.testing_mode = */ true,  // non default value
      /*.decompress_request = */ false,
  };
  static server::net::ParserStats test_stats;
  static server::request::ResponseDataAccounter test_accounter;
  std::vector<nghttp2_settings_entry> settings;
  server::http::Http2UpgradeData upgrade_data{.server_settings = settings};
  return server::http::HTTPHandlerBase(
      kTestHandlerInfoIndex, kTestRequestConfig, std::move(cb), test_stats,
      test_accounter, upgrade_data);
}

}  // namespace

namespace server::http {

using RequestBasePtr = std::shared_ptr<server::request::RequestBase>;

UTEST(UpgradeHttp2, TestWithUpgrade) {
  bool was_upgrade = false;

  auto func = [&was_upgrade](RequestBasePtr&& request_ptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& http_request = static_cast<HttpRequestImpl&>(*request_ptr);

    ASSERT_TRUE(http_request.HasUpgradeHeaders());

    if (http_request.HasUpgradeHeaders() &&
        HTTPHandlerBase::CheckUpgradeHeaders(http_request)) {
      was_upgrade = true;
    }
  };

  HTTPHandlerBase handler_base = CreateTestHttpHandlerBase(func);

  std::string data =
      "GET /index.html HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Connection: upgrade\r\n"
      "Upgrade: h2c\r\n"
      "HTTP2-Settings: ABCDEFGHIGKLMN\r\n\r\n";

  ASSERT_TRUE(handler_base.Parse(data.data(), data.size()));

  ASSERT_TRUE(was_upgrade);
}

UTEST(UpgradeHttp2, TestWithoutSettings) {
  bool was_upgrade = false;

  auto func = [&was_upgrade](RequestBasePtr&& request_ptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& http_request = static_cast<HttpRequestImpl&>(*request_ptr);

    ASSERT_TRUE(http_request.HasUpgradeHeaders());

    if (http_request.HasUpgradeHeaders() &&
        HTTPHandlerBase::CheckUpgradeHeaders(http_request)) {
      was_upgrade = true;
    }
  };

  HTTPHandlerBase handler_base = CreateTestHttpHandlerBase(func);

  std::string data =
      "GET /index.html HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Connection: upgrade\r\n"
      "Upgrade: h2c\r\n\r\n";

  ASSERT_TRUE(handler_base.Parse(data.data(), data.size()));

  ASSERT_FALSE(was_upgrade);
}

UTEST(UpgradeHttp2, TestWithDifferentProtocol) {
  bool was_upgrade = false;

  auto func = [&was_upgrade](RequestBasePtr&& request_ptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& http_request = static_cast<HttpRequestImpl&>(*request_ptr);

    ASSERT_TRUE(http_request.HasUpgradeHeaders());

    if (http_request.HasUpgradeHeaders() &&
        HTTPHandlerBase::CheckUpgradeHeaders(http_request)) {
      was_upgrade = true;
    }
  };

  HTTPHandlerBase handler_base = CreateTestHttpHandlerBase(func);

  std::string data =
      "GET /index.html HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Connection: upgrade\r\n"
      "Upgrade: h2\r\n"
      "HTTP2-Settings: ABCDEFGHIGKLMN\r\n\r\n";

  ASSERT_TRUE(handler_base.Parse(data.data(), data.size()));

  ASSERT_FALSE(was_upgrade);
}

UTEST(UpgradeHttp2, TestPOSTUpgrade) {
  bool was_upgrade = false;

  auto func = [&was_upgrade](RequestBasePtr&& request_ptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& http_request = static_cast<HttpRequestImpl&>(*request_ptr);

    ASSERT_TRUE(http_request.HasUpgradeHeaders());

    ASSERT_FALSE(http_request.RequestBody().empty());

    if (http_request.HasUpgradeHeaders() &&
        HTTPHandlerBase::CheckUpgradeHeaders(http_request)) {
      was_upgrade = true;
    }
  };

  HTTPHandlerBase handler_base = CreateTestHttpHandlerBase(func);

  std::string data =
      "POST /index.html HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Connection: Upgrade, HTTP2-Settings\r\n"
      "Upgrade: h2c\r\n"
      "HTTP2-Settings: ABCDEFGHIGKLMN\r\n"
      "Content-Length: 27\r\n\r\n"
      "field11value11field21value2";

  ASSERT_TRUE(handler_base.Parse(data.data(), data.size()));

  ASSERT_TRUE(was_upgrade);
}

}  // namespace server::http

USERVER_NAMESPACE_END
