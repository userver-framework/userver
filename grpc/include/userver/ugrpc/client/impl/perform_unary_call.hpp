#pragma once

#include <utility>

#include <userver/ugrpc/client/impl/unary_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Stub, typename Request, typename Response>
Response PerformUnaryCall(
    CallParams&& params,
    PrepareUnaryCallProxy<Stub, Request, Response>&& prepare_unary_call,
    const Request& request
) {
    UnaryCall unary_call{std::move(params), std::move(prepare_unary_call), request};
    unary_call.Perform();
    return unary_call.ExtractResponse();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
