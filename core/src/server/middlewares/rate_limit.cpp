#include <server/middlewares/rate_limit.hpp>

#include <server/handlers/http_handler_base_statistics.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

RateLimit::RateLimit(const handlers::HttpHandlerBase& handler)
    : rate_limit_{utils::TokenBucket::MakeUnbounded()},
      statistics_{handler.GetHandlerStatistics()},
      max_requests_per_second_{handler.GetConfig().max_requests_per_second},
      max_requests_in_flight_{handler.GetConfig().max_requests_in_flight},
      handler_{handler} {
  if (max_requests_per_second_.has_value()) {
    const auto max_rps = *max_requests_per_second_;
    UASSERT_MSG(
        max_rps > 0,
        "max_requests_per_second option was not verified in config parsing");
    rate_limit_.SetMaxSize(max_rps);
    rate_limit_.SetRefillPolicy(
        {1, utils::TokenBucket::Duration{std::chrono::seconds(1)} / max_rps});
  }
}

void RateLimit::HandleRequest(http::HttpRequest& request,
                              request::RequestContext& context) const {
  if (CheckRateLimit(request)) {
    Next(request, context);
  }
}

bool RateLimit::CheckRateLimit(const http::HttpRequest& request) const {
  auto& statistics = statistics_.ForMethod(request.GetMethod());

  const bool success = rate_limit_.Obtain();
  if (!success) {
    UASSERT(max_requests_per_second_);

    auto& response = request.GetHttpResponse();
    auto log_reason = fmt::format("reached max_requests_per_second={}",
                                  max_requests_per_second_.value_or(0));
    SetThrottleReason(
        response, std::move(log_reason),
        std::string{
            USERVER_NAMESPACE::http::headers::ratelimit_reason::kGlobal});
    statistics.IncrementRateLimitReached();

    FailProcessingAndSetResponse(request);
    return false;
  }

  auto max_requests_in_flight = max_requests_in_flight_;
  auto requests_in_flight = statistics.GetInFlight();
  if (max_requests_in_flight &&
      (requests_in_flight > *max_requests_in_flight)) {
    auto& http_response = request.GetHttpResponse();
    auto log_reason = fmt::format("reached max_requests_in_flight={}",
                                  *max_requests_in_flight);
    SetThrottleReason(
        http_response, std::move(log_reason),
        std::string{
            USERVER_NAMESPACE::http::headers::ratelimit_reason::kInFlight});

    statistics.IncrementTooManyRequestsInFlight();

    FailProcessingAndSetResponse(request);
    return false;
  }

  return true;
}

void RateLimit::FailProcessingAndSetResponse(
    const http::HttpRequest& request) const {
  const auto ex = handlers::ExceptionWithCode<
      handlers::HandlerErrorCode::kTooManyRequests>{};
  handler_.HandleCustomHandlerException(request, ex);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
