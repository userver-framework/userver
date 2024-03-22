#pragma once

#include <optional>

#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
class HttpHandlerStatistics;
}

namespace server::middlewares {

class RateLimit final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"userver-rate-limit-middleware"};

  explicit RateLimit(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  bool CheckRateLimit(const http::HttpRequest& request) const;

  void FailProcessingAndSetResponse(const http::HttpRequest& request) const;

  mutable utils::TokenBucket rate_limit_;
  handlers::HttpHandlerStatistics& statistics_;

  std::optional<std::size_t> max_requests_per_second_;
  std::optional<std::size_t> max_requests_in_flight_;

  const handlers::HttpHandlerBase& handler_;
};

using RateLimitFactory = SimpleHttpMiddlewareFactory<RateLimit>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
