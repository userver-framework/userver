#pragma once

#include <utility>

#include <userver/utils/trx_tracker.hpp>

#include <userver/ugrpc/client/impl/unary_call.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Response, typename Request>
Response PerformUnaryCall(CallParams&& params, const Request& request) {
    utils::trx_tracker::CheckNoTransactions(ugrpc::impl::GetCallName(params.method_path));
    UnaryCall<Request, Response> unary_call{std::move(params), request};
    return unary_call.Perform();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
