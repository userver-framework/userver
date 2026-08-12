#include "compression.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <zlib.h>

#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

namespace userver_httparena::middlewares {
namespace {
bool ShouldCompress(const server::http::HttpRequest& request) {
    const auto& accept_encoding = request.GetHeader(http::headers::kAcceptEncoding);
    return accept_encoding.find("gzip") != std::string_view::npos;
}

std::string CompressGzip(std::string_view input) {
    z_stream strm = {};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::string{input};
    }

    const auto bound = deflateBound(&strm, input.size());
    std::string output(bound, '\0');

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    strm.avail_in = input.size();
    strm.next_out = reinterpret_cast<Bytef*>(output.data());
    strm.avail_out = bound;

    const int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        return std::string{input};
    }

    output.resize(strm.total_out);
    deflateEnd(&strm);
    return output;
}
}  // namespace

CompressionMiddleware::CompressionMiddleware(const server::handlers::HttpHandlerBase&) {}

void CompressionMiddleware::HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& context)
    const {
    Next(request, context);

    auto& response = request.GetHttpResponse();
    if (!ShouldCompress(request)) {
        return;
    }
    if (response.IsBodyStreamed()) {
        return;
    }

    const auto& body = response.GetData();
    if (body.empty()) {
        return;
    }

    const auto compressed = CompressGzip(body);
    response.SetHeader(http::headers::kContentEncoding, "gzip");
    response.SetData(compressed);
}
}  // namespace userver_httparena::middlewares
