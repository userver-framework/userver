#pragma once

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace middlewares::impl {

// Interface for creating middlewares.
template <typename MiddlewareBase, typename HandlerInfo>
struct PipelineCreatorInterface {
    using Middlewares = std::vector<std::shared_ptr<const MiddlewareBase>>;

    virtual Middlewares CreateMiddlewares(const HandlerInfo& info) const = 0;
};

}  // namespace middlewares::impl

USERVER_NAMESPACE_END
