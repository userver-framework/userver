#include <userver/chaotic/openapi/client/content_encoding.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <userver/chaotic/openapi/client/command_control.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/compression/gzip.hpp>
#include <userver/compression/zstd.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace co = chaotic::openapi::client;

namespace {

using ContentEncoding = co::CommandControl::ContentEncoding;
using ContentEncodingLevel = co::CommandControl::ContentEncodingLevel;

constexpr int kFastGzipCompressionLevel = 2;
constexpr int kSlowGzipCompressionLevel = 8;
constexpr int kFastZstdCompressionLevel = -2;
constexpr int kSlowZstdCompressionLevel = 10;
constexpr std::string_view kGzipContentEncoding = "gzip";
constexpr std::string_view kZstdContentEncoding = "zstd";

struct EncodingParams {
    ContentEncoding encoding;
    ContentEncodingLevel level;
    std::optional<int> compression_level;
    std::string_view header;
    std::string_view test_name;
};

class RequestEncoding : public testing::TestWithParam<EncodingParams> {};

std::string Compress(std::string_view body, const EncodingParams& params) {
    switch (params.encoding) {
        case ContentEncoding::kAuto:
            throw std::runtime_error("Content encoding must be specified");
        case ContentEncoding::kGzip:
            return params.compression_level
                       ? compression::gzip::Compress(body, params.compression_level.value())
                       : compression::gzip::Compress(body);
        case ContentEncoding::kZstd:
            return params.compression_level
                       ? compression::zstd::Compress(body, params.compression_level.value())
                       : compression::zstd::Compress(body);
    }
    throw std::runtime_error("Unsupported content encoding");
}

std::string PrintEncodingParamsName(const testing::TestParamInfo<EncodingParams>& data) {
    return std::string{data.param.test_name};
}

class EmptyRequestEncoding : public testing::TestWithParam<ContentEncoding> {};

std::string PrintContentEncodingName(const testing::TestParamInfo<ContentEncoding>& data) {
    switch (data.param) {
        case ContentEncoding::kAuto:
            return "Auto";
        case ContentEncoding::kGzip:
            return "Gzip";
        case ContentEncoding::kZstd:
            return "Zstd";
    }
    throw std::runtime_error("Unsupported content encoding");
}

}  // namespace

UTEST(EncodeRequest, AutoIsNoop) {
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    request.url("http://localhost/").data("plain body");

    const co::CommandControl cc;
    EXPECT_EQ(cc.encoding, ContentEncoding::kAuto);
    EXPECT_EQ(cc.encoding_level, ContentEncodingLevel::kAuto);
    co::EncodeRequest(request, cc);

    EXPECT_EQ(request.GetData(), "plain body");
}

UTEST_P(RequestEncoding, CompressesBodyAndSetsHeader) {
    const auto& params = GetParam();
    const std::string original_body(4096, 'a');
    const auto expected_body = Compress(original_body, params);

    bool called = false;
    const utest::HttpServerMock
        http_server([&called, &expected_body, &params](const utest::HttpServerMock::HttpRequest& request) {
            constexpr http::headers::PredefinedHeader kContentEncoding("Content-Encoding");
            called = true;

            EXPECT_EQ(request.headers.at(kContentEncoding), params.header);
            EXPECT_EQ(request.body, expected_body);

            return utest::HttpServerMock::HttpResponse{};
        });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    request.post(http_server.GetBaseUrl(), original_body);

    const co::CommandControl cc{
        .encoding = params.encoding,
        .encoding_level = params.level,
    };
    co::EncodeRequest(request, cc);

    EXPECT_EQ(request.GetData(), expected_body);

    auto response = request.perform();
    EXPECT_TRUE(called);
}

INSTANTIATE_UTEST_SUITE_P(
    All,
    RequestEncoding,
    testing::Values(
        EncodingParams{
            ContentEncoding::kGzip,
            ContentEncodingLevel::kAuto,
            std::nullopt,
            kGzipContentEncoding,
            "GzipAuto"
        },
        EncodingParams{
            ContentEncoding::kGzip,
            ContentEncodingLevel::kFast,
            kFastGzipCompressionLevel,
            kGzipContentEncoding,
            "GzipFast"
        },
        EncodingParams{
            ContentEncoding::kGzip,
            ContentEncodingLevel::kSlow,
            kSlowGzipCompressionLevel,
            kGzipContentEncoding,
            "GzipSlow"
        },
        EncodingParams{
            ContentEncoding::kZstd,
            ContentEncodingLevel::kAuto,
            std::nullopt,
            kZstdContentEncoding,
            "ZstdAuto"
        },
        EncodingParams{
            ContentEncoding::kZstd,
            ContentEncodingLevel::kFast,
            kFastZstdCompressionLevel,
            kZstdContentEncoding,
            "ZstdFast"
        },
        EncodingParams{
            ContentEncoding::kZstd,
            ContentEncodingLevel::kSlow,
            kSlowZstdCompressionLevel,
            kZstdContentEncoding,
            "ZstdSlow"
        }
    ),
    PrintEncodingParamsName
);

UTEST_P(EmptyRequestEncoding, IsNoop) {
    bool called = false;
    const utest::HttpServerMock http_server([&called](const utest::HttpServerMock::HttpRequest& request) {
        constexpr http::headers::PredefinedHeader kContentEncoding("Content-Encoding");
        called = true;

        EXPECT_EQ(request.headers.count(kContentEncoding), 0);
        EXPECT_TRUE(request.body.empty());

        return utest::HttpServerMock::HttpResponse{};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    request.post(http_server.GetBaseUrl(), std::string{});

    const co::CommandControl cc{.encoding = GetParam()};
    co::EncodeRequest(request, cc);

    EXPECT_TRUE(request.GetData().empty());

    auto response = request.perform();
    EXPECT_TRUE(called);
}

INSTANTIATE_UTEST_SUITE_P(
    All,
    EmptyRequestEncoding,
    testing::Values(ContentEncoding::kGzip, ContentEncoding::kZstd),
    PrintContentEncodingName
);

USERVER_NAMESPACE_END
