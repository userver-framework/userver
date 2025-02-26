#pragma once

/// @file userver/ugrpc/client/response_future.hpp
/// @brief @copybrief ugrpc::client::ResponseFuture

#include <userver/ugrpc/client/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

template <typename Response>
class [[nodiscard]] ResponseFuture final {
public:
    /// @brief Checks if the asynchronous call has completed
    ///        Note, that once user gets result, IsReady should not be called
    /// @return true if result ready
    [[nodiscard]] bool IsReady() const noexcept;

    /// @brief Await response until specified timepoint
    ///
    /// @throws ugrpc::client::RpcError on an RPC error
    [[nodiscard]] engine::FutureStatus WaitUntil(engine::Deadline deadline) const;

    /// @brief Await and read the response
    ///
    /// `Get` should not be called multiple times for the same UnaryFuture.
    ///
    /// The connection is not closed, it will be reused for new RPCs.
    ///
    /// @returns the response on success
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    Response Get();

    /// @brief Get original gRPC Call
    CallAnyBase& GetCall();

    /// @cond
    // For internal use only
    template <typename PrepareFunc, typename Request>
    ResponseFuture(impl::CallParams&& params, PrepareFunc prepare_func, const Request& req);

    // For internal use only.
    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept;
    /// @endcond

private:
    impl::UnaryCall<Response> call_;
    std::unique_ptr<Response> response_;
    impl::UnaryFuture future_;
};

template <typename Response>
template <typename PrepareFunc, typename Request>
ResponseFuture<Response>::ResponseFuture(impl::CallParams&& params, PrepareFunc prepare_func, const Request& req)
    : call_(std::move(params), prepare_func, req),
      response_{std::make_unique<Response>()},
      future_{call_.FinishAsync(*response_)} {}

template <typename Response>
bool ResponseFuture<Response>::IsReady() const noexcept {
    return future_.IsReady();
}

template <typename Response>
engine::FutureStatus ResponseFuture<Response>::WaitUntil(engine::Deadline deadline) const {
    return future_.WaitUntil(deadline);
}

template <typename Response>
Response ResponseFuture<Response>::Get() {
    future_.Get();
    return std::move(*response_);
}

template <typename Response>
CallAnyBase& ResponseFuture<Response>::GetCall() {
    return call_;
}

template <typename Response>
engine::impl::ContextAccessor* ResponseFuture<Response>::TryGetContextAccessor() noexcept {
    return future_.TryGetContextAccessor();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
