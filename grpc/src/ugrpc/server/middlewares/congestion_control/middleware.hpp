#pragma once

#include <userver/server/congestion_control/limiter.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

struct Settings final {
    /// gRPC-code that the congestion control uses to reject requests
    grpc::StatusCode reject_status_code{grpc::StatusCode::RESOURCE_EXHAUSTED};
};

class Middleware final : public MiddlewareBase {
public:
    Middleware(const Settings& settings, std::shared_ptr<utils::TokenBucket> rate_limit);

    void OnCallStart(MiddlewareCallContext& context) const override;

private:
    std::shared_ptr<utils::TokenBucket> rate_limit_;
    Settings settings_;
};

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
