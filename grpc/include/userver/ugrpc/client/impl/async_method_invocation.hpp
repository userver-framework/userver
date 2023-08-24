#pragma once

#include <grpcpp/client_context.h>

#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ugrpc::impl::AsyncMethodInvocation::WaitStatus Wait(
    ugrpc::impl::AsyncMethodInvocation&, grpc::ClientContext& context) noexcept;

}

USERVER_NAMESPACE_END
