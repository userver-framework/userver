#include <userver/utest/utest.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <server/http/handler_info_index.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/net/stats.hpp>
#include <server/request/response_data_accounter.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/http/parser/http_request_parse_args.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_config.hpp>

USERVER_NAMESPACE_BEGIN

TEST(HttpRequestConstructor, DecodeUrl) {
    const std::string str = "Some+String%20x%30";
    EXPECT_EQ("Some String x0", http::parser::UrlDecode(str));
}

TEST(HttpRequestConstructor, DecodeUrlPlus) {
    const std::string str = "Some+String";
    EXPECT_EQ("Some String", http::parser::UrlDecode(str));
}

namespace {

constexpr std::string_view kMultipartContentType = "multipart/form-data; boundary=BOUND";

constexpr std::string_view kBodyThatArrivesCompressed =
    "--BOUND\r\n"
    "Content-Disposition: form-data; name=\"arg\"\r\n"
    "\r\n"
    "still-compressed\r\n"
    "--BOUND--\r\n";

constexpr std::string_view kBodyAfterDecompression =
    "--BOUND\r\n"
    "Content-Disposition: form-data; name=\"arg\"\r\n"
    "\r\n"
    "decompressed\r\n"
    "--BOUND--\r\n";

std::string MakeMultipartRequest(std::string_view body, bool compressed) {
    return fmt::format(
        "POST / HTTP/1.1\r\n"
        "Content-Type: {}\r\n"
        "{}"
        "Content-Length: {}\r\n"
        "\r\n"
        "{}",
        kMultipartContentType,
        compressed ? "Content-Encoding: gzip\r\n" : "",
        body.size(),
        body
    );
}

std::shared_ptr<server::http::HttpRequest> ParseRequest(std::string_view raw_request, bool decompress_request) {
    static const server::http::HandlerInfoIndex kHandlerInfoIndex;
    static server::net::ParserStats stats;
    static server::request::ResponseDataAccounter accounter;

    server::request::HttpRequestConfig config;
    config.testing_mode = true;
    config.decompress_request = decompress_request;

    std::shared_ptr<server::http::HttpRequest> parsed;
    server::http::HttpRequestParser parser{
        kHandlerInfoIndex,
        config,
        [&parsed](std::shared_ptr<server::http::HttpRequest>&& request) { parsed = std::move(request); },
        stats,
        accounter,
        engine::io::Sockaddr{},
    };
    EXPECT_TRUE(parser.Parse(raw_request));
    return parsed;
}

}  // namespace

UTEST(HttpRequestConstructor, MultipartFormDataArgs) {
    const auto request = ParseRequest(MakeMultipartRequest(kBodyAfterDecompression, false), true);

    ASSERT_TRUE(request);
    EXPECT_EQ(request->FormDataArgCount(), 1);
    EXPECT_EQ(request->GetFormDataArg("arg").value, "decompressed");
}

UTEST(HttpRequestConstructor, MultipartFormDataArgsOfCompressedBody) {
    const auto request = ParseRequest(MakeMultipartRequest(kBodyThatArrivesCompressed, true), true);

    ASSERT_TRUE(request);
    EXPECT_NE(request->GetHttpResponse().GetStatus(), server::http::HttpStatus::kBadRequest);
    EXPECT_EQ(request->FormDataArgCount(), 0);

    request->SetRequestBody(std::string{kBodyAfterDecompression});

    EXPECT_TRUE(request->ParseFormDataArgsFromBody());
    EXPECT_EQ(request->FormDataArgCount(), 1);
    EXPECT_EQ(request->GetFormDataArg("arg").value, "decompressed");
}

USERVER_NAMESPACE_END
