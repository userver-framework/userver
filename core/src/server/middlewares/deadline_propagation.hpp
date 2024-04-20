#pragma once

#include <userver/dynamic_config/source.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {
struct TaskInheritedData;
}

namespace server::middlewares {

class DeadlinePropagation final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{
      "userver-deadline-propagation-middleware"};

  explicit DeadlinePropagation(const handlers::HttpHandlerBase&);

 private:
  struct RequestScope;

  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  void SetUpInheritedData(const http::HttpRequest& request,
                          RequestScope& dp_scope) const;

  void SetupInheritedDeadline(const http::HttpRequest& request,
                              request::TaskInheritedData& inherited_data,
                              RequestScope& dp_scope) const;

  void HandleDeadlineExpired(const http::HttpRequest& request,
                             RequestScope& dp_scope,
                             std::string internal_message) const;

  void CompleteDeadlinePropagation(const http::HttpRequest& request,
                                   request::RequestContext& context,
                                   RequestScope& dp_scope) const;

  const handlers::HttpHandlerBase& handler_;
  const bool deadline_propagation_enabled_;
  const http::HttpStatus deadline_expired_status_code_;
  const std::string path_;
};

using DeadlinePropagationFactory =
    SimpleHttpMiddlewareFactory<DeadlinePropagation>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
