#include <userver/chaotic/openapi/client/content_encoding.hpp>

#include <userver/chaotic/openapi/client/command_control.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/compression/gzip.hpp>
#include <userver/http/common_headers.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

void EncodeRequest(clients::http::Request& request, const CommandControl& cc) {
    switch (cc.encoding) {
        using enum CommandControl::ContentEncoding;
        case kAuto:
            return;
        case kGzip: {
            auto body = request.ExtractData();
            if (body.empty()) {
                // Nothing to compress; avoid a pointless header + gzip overhead.
                request.data(std::move(body));
                return;
            }
            request.data(compression::gzip::Compress(body));
            request.headers({{USERVER_NAMESPACE::http::headers::kContentEncoding, "gzip"}});
            return;
        }
        default:
            throw std::runtime_error("Unsupported content encoding");
    }
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
