#pragma once

#include <userver/components/state.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::graceful_shutdown_headers {

class Middleware final : public MiddlewareBase {
public:
    Middleware(const components::State& state, dynamic_config::Source source);

    void OnCallStart(MiddlewareCallContext& context) const override;

private:
    const components::State state_;
    const dynamic_config::Source source_;
};

}  // namespace ugrpc::server::middlewares::graceful_shutdown_headers

USERVER_NAMESPACE_END
