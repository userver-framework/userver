#include <userver/chaotic/openapi/client/content_encoding.hpp>

#include <stdexcept>
#include <string_view>

#include <userver/chaotic/openapi/client/command_control.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/compression/gzip.hpp>
#include <userver/compression/zstd.hpp>
#include <userver/http/common_headers.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

namespace {

constexpr int kFastGzipCompressionLevel = 2;
constexpr int kSlowGzipCompressionLevel = 8;
constexpr int kFastZstdCompressionLevel = -2;
constexpr int kSlowZstdCompressionLevel = 10;

int GzipCompressLevelConversion(CommandControl::ContentEncodingLevel encoding_level) {
    switch (encoding_level) {
        using enum CommandControl::ContentEncodingLevel;
        case kAuto:
            return compression::gzip::kDefaultCompressionLevel;
        case kFast:
            return kFastGzipCompressionLevel;
        case kSlow:
            return kSlowGzipCompressionLevel;
    }
    throw std::runtime_error("Unsupported content encoding level");
}

int ZstdCompressLevelConversion(CommandControl::ContentEncodingLevel encoding_level) {
    switch (encoding_level) {
        using enum CommandControl::ContentEncodingLevel;
        case kAuto:
            return compression::zstd::kDefaultCompressionLevel;
        case kFast:
            return kFastZstdCompressionLevel;
        case kSlow:
            return kSlowZstdCompressionLevel;
    }
    throw std::runtime_error("Unsupported content encoding level");
}

template <CommandControl::ContentEncoding ContentEncoding>
struct ContentEncodingParams;

template <>
struct ContentEncodingParams<CommandControl::ContentEncoding::kGzip> {
    static constexpr auto kCompressFunc = compression::gzip::Compress;
    static constexpr auto kCompressLevelConversionFunc = GzipCompressLevelConversion;
    static constexpr std::string_view kContentEncoding = "gzip";
};

template <>
struct ContentEncodingParams<CommandControl::ContentEncoding::kZstd> {
    static constexpr auto kCompressFunc = compression::zstd::Compress;
    static constexpr auto kCompressLevelConversionFunc = ZstdCompressLevelConversion;
    static constexpr std::string_view kContentEncoding = "zstd";
};

template <CommandControl::ContentEncoding ContentEncoding>
void EncodeRequestBody(clients::http::Request& request, CommandControl::ContentEncodingLevel level) {
    using Params = ContentEncodingParams<ContentEncoding>;

    auto body = request.ExtractData();
    if (body.empty()) {
        request.data(std::move(body));
        return;
    }

    request.data(Params::kCompressFunc(body, Params::kCompressLevelConversionFunc(level)));
    request.headers({{USERVER_NAMESPACE::http::headers::kContentEncoding, Params::kContentEncoding}});
}

}  // namespace

void EncodeRequest(clients::http::Request& request, const CommandControl& cc) {
    switch (cc.encoding) {
        using enum CommandControl::ContentEncoding;
        case kAuto:
            return;
        case kGzip:
            EncodeRequestBody<kGzip>(request, cc.encoding_level);
            return;
        case kZstd:
            EncodeRequestBody<kZstd>(request, cc.encoding_level);
            return;
    }
    throw std::runtime_error("Unsupported content encoding");
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
