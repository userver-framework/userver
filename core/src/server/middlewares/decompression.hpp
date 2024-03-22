#pragma once

#include <userver/server/http/http_status.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {
class HttpResponse;
}

namespace server::handlers {
class CustomHandlerException;
}

namespace server::middlewares {

class Decompression final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"userver-decompression-middleware"};

  explicit Decompression(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  bool DecompressRequestBody(http::HttpRequest& request) const;

  const bool decompress_request_;
  const std::size_t max_request_size_;
  const bool parse_args_from_body_;

  const handlers::HttpHandlerBase& handler_;
};

using DecompressionFactory = SimpleHttpMiddlewareFactory<Decompression>;

class SetAcceptEncoding final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{
      "userver-set-accept-encoding-middleware"};

  explicit SetAcceptEncoding(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  void SetResponseAcceptEncoding(http::HttpResponse& response) const;

  const bool decompress_request_;
};

using SetAcceptEncodingFactory = SimpleHttpMiddlewareFactory<SetAcceptEncoding>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
