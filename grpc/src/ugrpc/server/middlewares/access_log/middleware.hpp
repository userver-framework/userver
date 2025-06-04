#pragma once

#include <userver/logging/fwd.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::access_log {

struct Settings final {
    logging::TextLoggerPtr access_tskv_logger;
};

class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(Settings&& settings);

    void OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const override;

private:
    logging::TextLoggerPtr logger_;
};

}  // namespace ugrpc::server::middlewares::access_log

USERVER_NAMESPACE_END
