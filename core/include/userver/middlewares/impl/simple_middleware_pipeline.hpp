#pragma once

#include <userver/middlewares/impl/pipeline_creator_interface.hpp>

USERVER_NAMESPACE_BEGIN

namespace middlewares::impl {

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
