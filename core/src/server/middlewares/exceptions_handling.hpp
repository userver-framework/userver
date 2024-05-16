#pragma once

#include <userver/server/middlewares/builtin.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

class ExceptionsHandling final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName = builtin::kExceptionsHandling;

  explicit ExceptionsHandling(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  const handlers::HttpHandlerBase& handler_;
};

using ExceptionsHandlingFactory =
    SimpleHttpMiddlewareFactory<ExceptionsHandling>;

class UnknownExceptionsHandling final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName = builtin::kUnknownExceptionsHandling;

  explicit UnknownExceptionsHandling(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  const handlers::HttpHandlerBase& handler_;
};

using UnknownExceptionsHandlingFactory =
    SimpleHttpMiddlewareFactory<UnknownExceptionsHandling>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
