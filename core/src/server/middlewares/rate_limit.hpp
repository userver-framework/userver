#pragma once

#include <limits>

#include <userver/server/middlewares/builtin.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
class HttpHandlerStatisticsAggregate;
}

namespace server::middlewares {

class RateLimit final : public HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = builtin::kRateLimit;

    explicit RateLimit(const handlers::HttpHandlerBase&);

private:
    static constexpr std::size_t kUnlimited = std::numeric_limits<std::size_t>::max();

    void HandleRequest(http::HttpRequest& request, request::RequestContext& context) const override;

    bool CheckRateLimit(const http::HttpRequest& request, request::RequestContext& context) const;

    void FailProcessingAndSetResponse(const http::HttpRequest& request, request::RequestContext& context) const;

    mutable utils::TokenBucket rate_limit_;
    handlers::HttpHandlerStatisticsAggregate& statistics_;

    const std::size_t max_requests_per_second_;
    const std::size_t max_requests_in_flight_;
    // False when neither RPS nor in-flight limits are configured — HandleRequest is a no-op.
    const bool checks_enabled_;

    const handlers::HttpHandlerBase& handler_;
};

using RateLimitFactory = SimpleHttpMiddlewareFactory<RateLimit>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
