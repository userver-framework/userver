#pragma once

/// @file userver/ugrpc/client/response_future.hpp
/// @brief @copybrief ugrpc::client::ResponseFuture

#include <memory>

#include <userver/ugrpc/client/impl/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Controls a single request -> single response RPC.
///
/// This class is not thread-safe, it cannot be used from multiple tasks at the same time.
///
/// The RPC is cancelled on destruction unless the RPC is already finished. In
/// that case the connection is not closed (it will be reused for new RPCs), and
/// the server receives `RpcInterruptedError` immediately.
template <typename Response>
class [[nodiscard]] ResponseFuture final {
    // Implementation note: further RPC controls (lazy Finish, ReadInitialMetadata), if needed in the future,
    // should be added to UnaryCall, not to ResponseFuture. In that case UnaryCall should be exposed in GetCall.
public:
    ResponseFuture(ResponseFuture&&) noexcept = default;
    ResponseFuture& operator=(ResponseFuture&&) noexcept = default;

    /// @brief Checks if the asynchronous call has completed
    ///        Note, that once user gets result, IsReady should not be called
    /// @return true if result ready
    [[nodiscard]] bool IsReady() const noexcept { return call_->GetFinishFuture().IsReady(); }

    /// @brief Await response until specified timepoint
    ///
    /// @throws ugrpc::client::RpcError on an RPC error
    [[nodiscard]] engine::FutureStatus WaitUntil(engine::Deadline deadline) const {
        return call_->GetFinishFuture().WaitUntil(deadline);
    }

    /// @brief Await and read the response
    ///
    /// `Get` should not be called multiple times for the same UnaryFuture.
    ///
    /// The connection is not closed, it will be reused for new RPCs.
    ///
    /// @returns the response on success
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    Response Get() { return call_->Finish(); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    CallContext& GetContext() { return call_->GetContext(); }

    /// @overload
    const CallContext& GetContext() const { return call_->GetContext(); }

    /// @cond
    // For internal use only
    template <typename Stub, typename Request>
    ResponseFuture(
        impl::CallParams&& params,
        impl::PrepareUnaryCallProxy<Stub, Request, Response>&& prepare_unary_call,
        const Request& request
    )
        : call_(std::make_unique<impl::UnaryCall<Response>>(std::move(params), std::move(prepare_unary_call), request)
          ) {}

    // For internal use only.
    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept {
        return call_->GetFinishFuture().TryGetContextAccessor();
    }
    /// @endcond

private:
    std::unique_ptr<impl::UnaryCall<Response>> call_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
