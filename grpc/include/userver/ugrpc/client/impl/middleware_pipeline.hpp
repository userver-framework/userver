#pragma once

#include <userver/ugrpc/client/impl/middleware_hooks.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class MiddlewarePipeline {
public:
    explicit MiddlewarePipeline(const Middlewares& middlewares) : middlewares_{middlewares} {}

    void Run(const MiddlewareHooks& hooks, MiddlewareCallContext& context) const;

private:
    const Middlewares& middlewares_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
