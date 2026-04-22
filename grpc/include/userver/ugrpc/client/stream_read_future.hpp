#pragma once

/// @file userver/ugrpc/client/stream_read_future.hpp
/// @brief @copybrief ugrpc::client::StreamReadFuture

#include <memory>
#include <utility>

#include <userver/ugrpc/client/impl/stream_read_future_impl_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief StreamReadFuture for waiting a single read response from stream
template <typename Response>
class [[nodiscard]] StreamReadFuture {
public:
    StreamReadFuture(StreamReadFuture&&) noexcept = default;
    StreamReadFuture& operator=(StreamReadFuture&&) noexcept = default;

    /// @brief Checks if the asynchronous call has completed
    ///        Note, that once user gets result, IsReady should not be called
    /// @return true if result ready
    [[nodiscard]] bool IsReady() const { return impl_->IsReady(); }

    /// @brief Await response
    ///
    /// Upon completion the result is available in `response` that was
    /// specified when initiating the asynchronous read
    ///
    /// `Get` should not be called multiple times for the same StreamReadFuture.
    ///
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    bool Get() { return impl_->Get(); }

    /// @cond
    // For internal use only
    explicit StreamReadFuture(std::unique_ptr<impl::StreamReadFutureImplBase<Response>> impl) noexcept
            : impl_{std::move(impl)}
    {}
    /// @endcond

private:
    std::unique_ptr<impl::StreamReadFutureImplBase<Response>> impl_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
