#pragma once

/// @file userver/ugrpc/tests/client_middleware_pipeline.hpp
/// @brief @copybrief ugrpc::tests::SimpleClientMiddlewarePipeline

#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

namespace impl {

using ClientPipeline = ugrpc::impl::SimpleMiddlewarePipeline<ugrpc::client::MiddlewareBase, ugrpc::client::ClientInfo>;

}  // namespace impl

class SimpleClientMiddlewarePipeline final : public impl::ClientPipeline {
public:
    SimpleClientMiddlewarePipeline();
};

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
