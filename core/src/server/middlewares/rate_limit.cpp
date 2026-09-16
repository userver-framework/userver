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
      max_requests_per_second_{handler.GetConfig().max_requests_per_second.value_or(kUnlimited)},
      max_requests_in_flight_{handler.GetConfig().max_requests_in_flight.value_or(kUnlimited)},
      checks_enabled_{max_requests_per_second_ != kUnlimited || max_requests_in_flight_ != kUnlimited},
      handler_{handler}
{
    if (max_requests_per_second_ != kUnlimited) {
        UASSERT_MSG(max_requests_per_second_ > 0, "max_requests_per_second option was not verified in config parsing");
        rate_limit_.SetMaxSize(max_requests_per_second_);
        rate_limit_
            .SetRefillPolicy({1, utils::TokenBucket::Duration{std::chrono::seconds(1)} / max_requests_per_second_});
    }
}

void RateLimit::HandleRequest(http::HttpRequest& request, request::RequestContext& context) const {
    if (!checks_enabled_ || CheckRateLimit(request, context)) {
        Next(request, context);
    }
}

bool RateLimit::CheckRateLimit(const http::HttpRequest& request, request::RequestContext& context) const {
    auto& statistics = statistics_.GetOverallStatistics().ForMethod(request.GetMethod());

    if (max_requests_per_second_ != kUnlimited && !rate_limit_.Obtain()) {
        auto& response = request.GetHttpResponse();
        auto log_reason = fmt::format("reached max_requests_per_second={}", max_requests_per_second_);
        SetThrottleReason(
            response,
            std::move(log_reason),
            std::string{USERVER_NAMESPACE::http::headers::ratelimit_reason::kGlobal}
        );
        statistics.IncrementRateLimitReached();

        FailProcessingAndSetResponse(request, context);
        return false;
    }

    if (max_requests_in_flight_ != kUnlimited && statistics.GetInFlight() > max_requests_in_flight_) {
        auto& http_response = request.GetHttpResponse();
        auto log_reason = fmt::format("reached max_requests_in_flight={}", max_requests_in_flight_);
        SetThrottleReason(
            http_response,
            std::move(log_reason),
            std::string{USERVER_NAMESPACE::http::headers::ratelimit_reason::kInFlight}
        );

        statistics.IncrementTooManyRequestsInFlight();

        FailProcessingAndSetResponse(request, context);
        return false;
    }

    return true;
}

void RateLimit::FailProcessingAndSetResponse(const http::HttpRequest& request, request::RequestContext& context) const {
    const auto ex = handlers::ExceptionWithCode<handlers::HandlerErrorCode::kTooManyRequests>{};
    handler_.HandleCustomHandlerException(request, context, ex);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
