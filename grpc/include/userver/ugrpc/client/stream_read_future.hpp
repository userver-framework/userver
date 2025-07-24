#pragma once

/// @file userver/ugrpc/client/stream_read_future.hpp
/// @brief @copybrief ugrpc::client::StreamReadFuture

#include <utility>

#include <google/protobuf/message.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/impl/async_stream_methods.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief StreamReadFuture for waiting a single read response from stream
template <typename RawStream>
class [[nodiscard]] StreamReadFuture {
public:
    /// @cond
    StreamReadFuture(
        impl::StreamingCallState& state,
        RawStream& stream,
        const google::protobuf::Message* recv_message
    ) noexcept;
    /// @endcond

    StreamReadFuture(StreamReadFuture&& other) noexcept;
    StreamReadFuture& operator=(StreamReadFuture&& other) noexcept;
    StreamReadFuture(const StreamReadFuture&) = delete;
    StreamReadFuture& operator=(const StreamReadFuture&) = delete;

    ~StreamReadFuture();

    /// @brief Await response
    ///
    /// Upon completion the result is available in `response` that was
    /// specified when initiating the asynchronous read
    ///
    /// `Get` should not be called multiple times for the same StreamReadFuture.
    ///
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    bool Get();

    /// @brief Checks if the asynchronous call has completed
    ///        Note, that once user gets result, IsReady should not be called
    /// @return true if result ready
    [[nodiscard]] bool IsReady() const noexcept;

private:
    impl::StreamingCallState* state_{};
    RawStream* stream_{};
    const google::protobuf::Message* recv_message_;
};

template <typename RawStream>
StreamReadFuture<RawStream>::StreamReadFuture(
    impl::StreamingCallState& state,
    RawStream& stream,
    const google::protobuf::Message* recv_message
) noexcept
    : state_{&state}, stream_{&stream}, recv_message_{recv_message} {}

template <typename RawStream>
StreamReadFuture<RawStream>::StreamReadFuture(StreamReadFuture&& other) noexcept
    // state_ == nullptr signals that *this is empty. Other fields may remain garbage in `other`.
    : state_{std::exchange(other.state_, nullptr)}, stream_{other.stream_}, recv_message_{other.recv_message_} {}

template <typename RawStream>
StreamReadFuture<RawStream>& StreamReadFuture<RawStream>::operator=(StreamReadFuture&& other) noexcept {
    if (this == &other) return *this;
    [[maybe_unused]] auto for_destruction = std::move(*this);
    // state_ == nullptr signals that *this is empty. Other fields may remain garbage in `other`.
    state_ = std::exchange(other.state_, nullptr);
    stream_ = other.stream_;
    recv_message_ = other.recv_message_;
    return *this;
}

template <typename RawStream>
StreamReadFuture<RawStream>::~StreamReadFuture() {
    if (state_) {
        // StreamReadFuture::Get wasn't called => finish RPC.
        impl::FinishAbandoned(*stream_, *state_);
    }
}

template <typename RawStream>
bool StreamReadFuture<RawStream>::Get() {
    UINVARIANT(state_, "'Get' must be called only once");
    const impl::StreamingCallState::AsyncMethodInvocationGuard guard(*state_);
    auto* const state = std::exchange(state_, nullptr);
    const auto result = impl::WaitAndTryCancelIfNeeded(state->GetAsyncMethodInvocation(), state->GetClientContext());
    if (result == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state->GetStatsScope().OnCancelled();
        state->GetStatsScope().Flush();
    } else if (result == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError) {
        // Finish can only be called once all the data is read, otherwise the
        // underlying gRPC driver hangs.
        impl::Finish(*stream_, *state, /*final_response=*/nullptr, /*throw_on_error=*/true);
    } else {
        if (recv_message_) {
            impl::MiddlewarePipeline::PostRecvMessage(*state, *recv_message_);
        }
    }
    return result == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename RawStream>
bool StreamReadFuture<RawStream>::IsReady() const noexcept {
    UINVARIANT(state_, "IsReady should be called only before 'Get'");
    auto& method = state_->GetAsyncMethodInvocation();
    return method.IsReady();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
