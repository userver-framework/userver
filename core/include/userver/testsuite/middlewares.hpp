#pragma once

#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

class ExceptionsHandlingMiddleware final
    : public server::middlewares::HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{
      "testsuite-exceptions-handling-middleware"};

  explicit ExceptionsHandlingMiddleware(
      const server::handlers::HttpHandlerBase&) {}

 private:
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context) const override;
};

using ExceptionsHandlingMiddlewareFactory =
    server::middlewares::SimpleHttpMiddlewareFactory<
        ExceptionsHandlingMiddleware>;

}  // namespace testsuite

USERVER_NAMESPACE_END
