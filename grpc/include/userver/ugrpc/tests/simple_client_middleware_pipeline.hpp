#pragma once

/// @file userver/ugrpc/tests/simple_client_middleware_pipeline.hpp
/// @brief @copybrief ugrpc::tests::SimpleClientMiddlewarePipeline

#include <userver/middlewares/impl/simple_middleware_pipeline.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

using SimpleClientMiddlewarePipeline = USERVER_NAMESPACE::middlewares::impl::
    SimpleMiddlewarePipeline<ugrpc::client::MiddlewareBase, ugrpc::client::ClientInfo>;

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
