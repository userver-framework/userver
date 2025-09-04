#include <ugrpc/server/middlewares/congestion_control/middleware.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

namespace {

bool CheckRatelimit(utils::TokenBucket& rate_limit, std::string_view call_name) {
    if (rate_limit.Obtain()) {
        return true;
    }

    LOG_LIMITED_ERROR() << "Request throttled (congestion control, "
                           "limit via USERVER_RPS_CCONTROL and USERVER_RPS_CCONTROL_ENABLED), "
                        << "limit=" << rate_limit.GetRatePs() << "/sec, "
                        << "service/method=" << call_name;

    return false;
}

}  // namespace

Middleware::Middleware(const Settings& settings, std::shared_ptr<utils::TokenBucket> rate_limit)
    : rate_limit_(std::move(rate_limit)), settings_(settings) {}

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    if (!CheckRatelimit(*rate_limit_, context.GetCallName())) {
        auto& server_context = context.GetServerContext();

        server_context.AddInitialMetadata(ugrpc::impl::kXYaTaxiRatelimitedBy, ugrpc::impl::kHostname);
        server_context.AddInitialMetadata(
            ugrpc::impl::kXYaTaxiRatelimitReason, ugrpc::impl::kCongestionControlRatelimitReason
        );

        return context.SetError(grpc::Status{settings_.reject_status_code, "Congestion control: rate limit exceeded"});
    }
}

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
