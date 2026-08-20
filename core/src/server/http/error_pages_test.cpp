#include <server/http/error_pages.hpp>

#include <string>
#include <string_view>

#include <gmock/gmock.h>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using server::http::HttpStatus;

server::http::ErrorPages ParseErrorPages(std::string_view yaml) {
    return yaml_config::YamlConfig{formats::yaml::FromString(std::string{yaml}), {}}.As<server::http::ErrorPages>();
}

constexpr std::string_view kTwoPages = R"(
- statuses: [404, 405]
  status: 200
  body: "<html>index</html>"
  headers:
      Content-Type: text/html
      X-Powered-By: WEETS-WA
- statuses: [500]
  body: "oops"
)";

}  // namespace

TEST(ErrorPages, Empty) {
    const auto pages = ParseErrorPages("[]");
    EXPECT_EQ(pages.Find(HttpStatus::kNotFound), nullptr);
}

TEST(ErrorPages, FindsOnlyConfiguredStatuses) {
    const auto pages = ParseErrorPages(kTwoPages);

    ASSERT_NE(pages.Find(HttpStatus::kNotFound), nullptr);
    ASSERT_NE(pages.Find(HttpStatus::kMethodNotAllowed), nullptr);
    ASSERT_NE(pages.Find(HttpStatus::kInternalServerError), nullptr);
    EXPECT_EQ(pages.Find(HttpStatus::kBadRequest), nullptr);
    EXPECT_EQ(pages.Find(HttpStatus::kOk), nullptr);
}

TEST(ErrorPages, ParsesFields) {
    const auto pages = ParseErrorPages(kTwoPages);

    const auto* page = pages.Find(HttpStatus::kMethodNotAllowed);
    ASSERT_NE(page, nullptr);
    ASSERT_TRUE(page->status);
    EXPECT_EQ(*page->status, HttpStatus::kOk);
    ASSERT_TRUE(page->body);
    EXPECT_EQ(*page->body, "<html>index</html>");
    // The headers keep the configuration order.
    EXPECT_THAT(
        page->headers,
        testing::ElementsAre(testing::Pair("Content-Type", "text/html"), testing::Pair("X-Powered-By", "WEETS-WA"))
    );

    const auto* server_error_page = pages.Find(HttpStatus::kInternalServerError);
    ASSERT_NE(server_error_page, nullptr);
    EXPECT_FALSE(server_error_page->status);
    ASSERT_TRUE(server_error_page->body);
    EXPECT_EQ(*server_error_page->body, "oops");
    EXPECT_TRUE(server_error_page->headers.empty());
}

TEST(ErrorPages, BodyFromFile) {
    auto file = fs::blocking::TempFile::Create();
    constexpr std::string_view kContents = "<html>from file</html>";
    fs::blocking::RewriteFileContents(file.GetPath(), kContents);

    const auto pages = ParseErrorPages(fmt::format("- statuses: [404]\n  body-path: {}\n", file.GetPath()));

    const auto* page = pages.Find(HttpStatus::kNotFound);
    ASSERT_NE(page, nullptr);
    ASSERT_TRUE(page->body);
    EXPECT_EQ(*page->body, kContents);
}

TEST(ErrorPages, RejectsInvalidConfigs) {
    // A page that would change nothing is a configuration mistake.
    EXPECT_THROW(ParseErrorPages("- statuses: [404]"), std::runtime_error);
    EXPECT_THROW(ParseErrorPages("- statuses: []\n  status: 200"), std::runtime_error);
    EXPECT_THROW(ParseErrorPages("- status: 200"), std::runtime_error);
    // 'body' and 'body-path' are mutually exclusive.
    EXPECT_THROW(ParseErrorPages("- statuses: [404]\n  body: a\n  body-path: /dev/null"), std::runtime_error);
    EXPECT_THROW(ParseErrorPages("- statuses: [404]\n  body-path: /no/such/file"), std::runtime_error);
    // Only errors are reported by the server itself, so only they can be substituted.
    EXPECT_THROW(ParseErrorPages("- statuses: [200]\n  status: 204"), std::runtime_error);
    EXPECT_THROW(ParseErrorPages("- statuses: [600]\n  status: 204"), std::runtime_error);
    EXPECT_THROW(ParseErrorPages("- statuses: [404]\n  status: 42"), std::runtime_error);
    // A header that HttpResponse would reject must be rejected at start.
    EXPECT_THROW(ParseErrorPages("- statuses: [404]\n  headers:\n      'Bad Name': v"), std::runtime_error);
    EXPECT_THROW(ParseErrorPages("- statuses: [404]\n  headers:\n      Name: \"bad\\rvalue\""), std::runtime_error);
    // A status must not be claimed by two pages.
    EXPECT_THROW(
        ParseErrorPages("- statuses: [404, 405]\n  status: 200\n- statuses: [405]\n  status: 201"), std::runtime_error
    );
}

UTEST(ErrorPages, ApplyOverridesStatusBodyAndHeaders) {
    server::request::ResponseDataAccounter accounter;
    const auto request = server::http::HttpRequestBuilder{accounter}.Build();
    auto& response = request->GetHttpResponse();
    response.SetStatus(HttpStatus::kMethodNotAllowed);
    response.SetData("method not allowed");

    const auto pages = ParseErrorPages(kTwoPages);
    const auto* page = pages.Find(HttpStatus::kMethodNotAllowed);
    ASSERT_NE(page, nullptr);
    server::http::ApplyErrorPage(*page, response);

    EXPECT_EQ(response.GetStatus(), HttpStatus::kOk);
    EXPECT_EQ(response.GetData(), "<html>index</html>");
    EXPECT_EQ(response.GetHeader("Content-Type"), "text/html");
    EXPECT_EQ(response.GetHeader("X-Powered-By"), "WEETS-WA");
}

UTEST(ErrorPages, ApplyKeepsWhatIsNotConfigured) {
    server::request::ResponseDataAccounter accounter;
    const auto request = server::http::HttpRequestBuilder{accounter}.Build();
    auto& response = request->GetHttpResponse();
    response.SetStatus(HttpStatus::kInternalServerError);
    response.SetData("internal server error");

    const auto pages = ParseErrorPages(kTwoPages);
    const auto* page = pages.Find(HttpStatus::kInternalServerError);
    ASSERT_NE(page, nullptr);
    server::http::ApplyErrorPage(*page, response);

    EXPECT_EQ(response.GetStatus(), HttpStatus::kInternalServerError);
    EXPECT_EQ(response.GetData(), "oops");
}

USERVER_NAMESPACE_END
