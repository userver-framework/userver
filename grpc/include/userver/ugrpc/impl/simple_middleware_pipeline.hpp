#pragma once

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

// Interface for creating middlewares.
template <typename MiddlewareBase, typename HandlerInfo>
struct PipelineCreatorInterface {
    using Middlewares = std::vector<std::shared_ptr<MiddlewareBase>>;

    virtual std::vector<std::shared_ptr<MiddlewareBase>> CreateMiddlewares(const HandlerInfo& info) const = 0;
};

// Simple impl of PipelineCreatorInterface for tests.
template <typename MiddlewareBase, typename HandlerInfo>
class SimpleMiddlewarePipeline : public PipelineCreatorInterface<MiddlewareBase, HandlerInfo> {
public:
    using Middlewares = typename PipelineCreatorInterface<MiddlewareBase, HandlerInfo>::Middlewares;

    SimpleMiddlewarePipeline(Middlewares&& mids) : mids_(std::move(mids)) {}

    Middlewares CreateMiddlewares(const HandlerInfo& /*handler_info*/
    ) const override {
        return mids_;
    }

    void SetMiddlewares(Middlewares&& middlewares) { mids_ = std::move(middlewares); }

private:
    Middlewares mids_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
