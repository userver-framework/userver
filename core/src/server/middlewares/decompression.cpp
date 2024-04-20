#include <server/middlewares/decompression.hpp>

#include <compression/gzip.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

namespace {
bool GetDecompressRequestFromHandlerSettings(
    const handlers::HttpHandlerBase& handler) {
  return handler.GetConfig().decompress_request;
}
}  // namespace

Decompression::Decompression(const handlers::HttpHandlerBase& handler)
    : decompress_request_{GetDecompressRequestFromHandlerSettings(handler)},
      max_request_size_{handler.GetConfig().request_config.max_request_size},
      parse_args_from_body_{
          handler.GetConfig().request_config.parse_args_from_body},
      handler_{handler} {}

void Decompression::HandleRequest(http::HttpRequest& request,
                                  request::RequestContext& context) const {
  if (DecompressRequestBody(request)) {
    Next(request, context);
  }
}

bool Decompression::DecompressRequestBody(http::HttpRequest& request) const {
  if (!decompress_request_ || !request.IsBodyCompressed()) {
    return true;
  }

  const auto scope_time = tracing::ScopeTime::CreateOptionalScopeTime(
      "http_decompress_request_body");

  const auto& content_encoding =
      request.GetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding);
  const utils::FastScopeGuard encoding_remove_guard{[&request]() noexcept {
    request.RemoveHeader(USERVER_NAMESPACE::http::headers::kContentEncoding);
  }};

  try {
    if (content_encoding == "gzip") {
      auto body = compression::gzip::Decompress(request.RequestBody(),
                                                max_request_size_);
      request.SetRequestBody(std::move(body));
      if (parse_args_from_body_) {
        request.ParseArgsFromBody();
      }
      return true;
    }
  } catch (const compression::TooBigError&) {
    handler_.HandleCustomHandlerException(
        request,
        handlers::ClientError{handlers::HandlerErrorCode::kPayloadTooLarge});
    return false;
  } catch (const std::exception& e) {
    handler_.HandleCustomHandlerException(
        request,
        handlers::RequestParseError{handlers::InternalMessage{
            fmt::format("Failed to decompress request body: {}", e.what())}});
    return false;
  }

  handler_.HandleCustomHandlerException(
      request,
      handlers::ClientError{handlers::HandlerErrorCode::kUnsupportedMediaType});
  return false;
}

SetAcceptEncoding::SetAcceptEncoding(const handlers::HttpHandlerBase& handler)
    : decompress_request_{GetDecompressRequestFromHandlerSettings(handler)} {}

void SetAcceptEncoding::HandleRequest(http::HttpRequest& request,
                                      request::RequestContext& context) const {
  const utils::ScopeGuard set_accept_encoding_scope{[this, &request] {
    SetResponseAcceptEncoding(request.GetHttpResponse());
  }};

  Next(request, context);
}

void SetAcceptEncoding::SetResponseAcceptEncoding(
    http::HttpResponse& response) const {
  if (!decompress_request_) return;

  // RFC7694, 3.
  // This specification expands that definition to allow "Accept-Encoding"
  // as a response header field as well.  When present in a response, it
  // indicates what content codings the resource was willing to accept in
  // the associated request.

  // User didn't set Accept-Encoding, let us do that
  if (!response.HasHeader(USERVER_NAMESPACE::http::headers::kAcceptEncoding)) {
    response.SetHeader(USERVER_NAMESPACE::http::headers::kAcceptEncoding,
                       "gzip, identity");
  }
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
