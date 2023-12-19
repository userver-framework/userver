#include "middleware.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

namespace {

bool CheckRatelimit(utils::TokenBucket& rate_limit,
                    std::string_view call_name) {
  if (rate_limit.Obtain()) {
    return true;
  }

  LOG_LIMITED_ERROR()
      << "Request throttled (congestion control, "
         "limit via USERVER_RPS_CCONTROL and USERVER_RPS_CCONTROL_ENABLED), "
      << "limit=" << rate_limit.GetRatePs() << "/sec, "
      << "service/method=" << call_name;

  return false;
}

}  // namespace

void Middleware::SetLimit(std::optional<size_t> new_limit) {
  if (new_limit) {
    const auto rps_val = *new_limit;
    if (rps_val > 0) {
      rate_limit_.SetMaxSize(rps_val);
      rate_limit_.SetRefillPolicy(
          {1, utils::TokenBucket::Duration{std::chrono::seconds(1)} / rps_val});
    } else {
      rate_limit_.SetMaxSize(0);
    }
  } else {
    rate_limit_.SetMaxSize(1);  // in case it was zero
    rate_limit_.SetInstantRefillPolicy();
  }
}

void Middleware::Handle(MiddlewareCallContext& context) const {
  auto& call = context.GetCall();

  if (!CheckRatelimit(rate_limit_, context.GetCall().GetCallName())) {
    call.FinishWithError(
        grpc::Status{grpc::StatusCode::RESOURCE_EXHAUSTED,
                     "Congestion control: rate limit exceeded"});
    return;
  }

  context.Next();
}

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
