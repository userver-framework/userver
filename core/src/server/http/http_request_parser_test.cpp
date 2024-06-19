#include <server/http/http_request_parser.hpp>

#include <server/http/create_parser_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kHttpRequestSmall = "GET / HTTP/1.1\r\n\r\n";

constexpr std::string_view kHttpRequestOriginUrl =
    "GET /foo/bar?query1=value1&query2=value2 HTTP/1.1\r\n\r\n";

constexpr std::string_view kHttpRequestAbsoluteUrl =
    "GET http://www.example.org/pub/WWW/TheProject.html HTTP/1.1\r\n\r\n";

constexpr std::string_view kHttpRequestHeadersSimple =
    "GET / HTTP/1.1\r\n"
    "Host: localhost:11235\r\nUser-Agent: curl/7.58.0\r\n\r\n";

constexpr std::string_view kHttpRequestHeadersNoSpaces =
    "GET / HTTP/1.1\r\n"
    "Host:localhost:11235\r\nUser-Agent:curl/7.58.0\r\n\r\n";

constexpr std::string_view kHttpRequestHeadersCaseInsensitive =
    "GET / HTTP/1.1\r\n"
    "hOst: localhost:11235\r\nuser-AGENT: curl/7.58.0\r\n\r\n";

constexpr std::string_view kHttpRequestHeaderValues =
    "GET / HTTP/1.1\r\n"
    "Host: *\"@!%\r\nUser-Agent: [-]{~},/\r\n\r\n";

constexpr std::string_view kHttpRequestBodySimple =
    "GET / HTTP/1.1\r\n"
    "Host: localhost:11235\r\nContent-Length: 4\r\n\r\n"
    "body";

}  // namespace

UTEST(HttpRequestParserParser, Small) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.GetUrl(), "/");

        EXPECT_EQ(http_request_impl.GetHttpMajor(), 1);
        EXPECT_EQ(http_request_impl.GetHttpMinor(), 1);
      });

  parser.Parse(kHttpRequestSmall.data(), kHttpRequestSmall.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, OriginUrl) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.GetUrl(),
                  "/foo/bar?query1=value1&query2=value2");
        EXPECT_EQ(http_request_impl.GetRequestPath(), "/foo/bar");
      });

  parser.Parse(kHttpRequestOriginUrl.data(), kHttpRequestOriginUrl.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, AbsoluteUrl) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.GetUrl(),
                  "http://www.example.org/pub/WWW/TheProject.html");
        EXPECT_EQ(http_request_impl.GetRequestPath(),
                  "/pub/WWW/TheProject.html");
      });

  parser.Parse(kHttpRequestAbsoluteUrl.data(), kHttpRequestAbsoluteUrl.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, HeadersSimple) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.HeaderCount(), 2);
        EXPECT_EQ(http_request_impl.HasHeader("host"), true);
        EXPECT_EQ(http_request_impl.GetHeader("host"), "localhost:11235");

        EXPECT_EQ(http_request_impl.HasHeader("user-agent"), true);
        EXPECT_EQ(http_request_impl.GetHeader("user-agent"), "curl/7.58.0");
      });

  parser.Parse(kHttpRequestHeadersSimple.data(),
               kHttpRequestHeadersSimple.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, HeadersNoSpaces) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.HeaderCount(), 2);
        EXPECT_EQ(http_request_impl.HasHeader("host"), true);
        EXPECT_EQ(http_request_impl.GetHeader("host"), "localhost:11235");

        EXPECT_EQ(http_request_impl.HasHeader("user-agent"), true);
        EXPECT_EQ(http_request_impl.GetHeader("user-agent"), "curl/7.58.0");
      });

  parser.Parse(kHttpRequestHeadersNoSpaces.data(),
               kHttpRequestHeadersNoSpaces.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, HeadersCaseInsensitive) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.HeaderCount(), 2);
        EXPECT_EQ(http_request_impl.HasHeader("host"), true);
        EXPECT_EQ(http_request_impl.GetHeader("host"), "localhost:11235");

        EXPECT_EQ(http_request_impl.HasHeader("user-agent"), true);
        EXPECT_EQ(http_request_impl.GetHeader("user-agent"), "curl/7.58.0");
      });

  parser.Parse(kHttpRequestHeadersCaseInsensitive.data(),
               kHttpRequestHeadersCaseInsensitive.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, HeaderValues) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.HeaderCount(), 2);
        EXPECT_EQ(http_request_impl.HasHeader("host"), true);
        EXPECT_EQ(http_request_impl.GetHeader("host"), "*\"@!%");

        EXPECT_EQ(http_request_impl.HasHeader("user-agent"), true);
        EXPECT_EQ(http_request_impl.GetHeader("user-agent"), "[-]{~},/");
      });

  parser.Parse(kHttpRequestHeaderValues.data(),
               kHttpRequestHeaderValues.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, BodySimple) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kGet);

        EXPECT_EQ(http_request_impl.RequestBody(), "body");
      });

  parser.Parse(kHttpRequestBodySimple.data(), kHttpRequestBodySimple.size());
  EXPECT_EQ(parsed, true);
}

// bad requests

namespace {

constexpr std::string_view kHttpRequestMethodWrongCase =
    "GeT / HTTP/1.1\r\n\r\n";

constexpr std::string_view kHttpRequestNoURL = "GET  HTTP/1.1\r\n\r\n";

constexpr std::string_view kHttpRequestAbsentCRLF = "GET / HTTP/1.1\r\n";

constexpr std::string_view kHttpRequestBodyContentLengthTooLong =
    "GET / HTTP/1.1\r\n"
    "Host: localhost:11235\r\nContent-Length: 5\r\n\r\n"
    "body";

}  // namespace

UTEST(HttpRequestParserParser, MethodWrongCase) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kUnknown);
      });

  parser.Parse(kHttpRequestMethodWrongCase.data(),
               kHttpRequestMethodWrongCase.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, NoURL) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        EXPECT_EQ(http_request_impl.GetMethod(),
                  server::http::HttpMethod::kUnknown);
      });

  parser.Parse(kHttpRequestNoURL.data(), kHttpRequestNoURL.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestParserParser, AbsentCRLF) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&&) {
        parsed = true;
      });

  parser.Parse(kHttpRequestAbsentCRLF.data(), kHttpRequestAbsentCRLF.size());
  EXPECT_EQ(parsed, false);
}

UTEST(HttpRequestParserParser, BodyContentLengthTooLong) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&&) {
        parsed = true;
      });

  parser.Parse(kHttpRequestBodyContentLengthTooLong.data(),
               kHttpRequestBodyContentLengthTooLong.size());
  EXPECT_EQ(parsed, false);
}

USERVER_NAMESPACE_END
