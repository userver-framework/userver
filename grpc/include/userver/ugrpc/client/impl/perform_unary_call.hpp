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
    utils::trx_tracker::CheckNoTransactions(params.call_name.Get());
    UnaryCall unary_call{std::move(params), std::move(prepare_unary_call), request};
    return unary_call.Perform();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
