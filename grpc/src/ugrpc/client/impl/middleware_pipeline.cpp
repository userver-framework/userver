#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void MiddlewarePipeline::Run(const MiddlewareHooks& hooks, MiddlewareCallContext& context) const {
    try {
        for (const auto& m : middlewares_) {
            hooks.Run(*m, context);
        }
    } catch (const std::exception& ex) {
        LOG_WARNING() << "Run middlewares failed: " << ex;
        throw;
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
