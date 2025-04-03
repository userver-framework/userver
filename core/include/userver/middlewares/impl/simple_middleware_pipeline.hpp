#pragma once

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace middlewares::impl {

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

    SimpleMiddlewarePipeline() = default;

    explicit SimpleMiddlewarePipeline(Middlewares&& middlewares) : middlewares_(std::move(middlewares)) {}

    Middlewares CreateMiddlewares(const HandlerInfo& /*handler_info*/) const override { return middlewares_; }

    void SetMiddlewares(Middlewares&& middlewares) { middlewares_ = std::move(middlewares); }

private:
    Middlewares middlewares_;
};

}  // namespace middlewares::impl

USERVER_NAMESPACE_END
