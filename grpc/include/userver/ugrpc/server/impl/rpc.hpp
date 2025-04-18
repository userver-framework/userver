#pragma once

#include <google/protobuf/message.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/deadline_timepoint.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/call.hpp>
#include <userver/ugrpc/server/stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Controls an RPC, server-side.
///
/// This class allows the following concurrent calls:
///
///   - `GetContext`
///   - `Read`;
///   - one of (`Write`, `Finish`, `FinishWithError`).
///
/// The RPC is cancelled on destruction unless the stream has been finished.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename CallTraits>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class Call final : public CallAnyBase, public CallTraits::StreamAdapter {
    using Request = typename CallTraits::Request;
    using Response = typename CallTraits::Response;
    using RawCall = typename CallTraits::RawCall;
    static constexpr CallKind kCallKind = CallTraits::kCallKind;

public:
    Call(CallParams&& call_params, RawCall& stream);

    Call(Call&&) = delete;
    Call& operator=(Call&&) = delete;
    ~Call() override;

    bool IsFinished() const { return is_finished_; }

    /// @brief Await and read the next incoming message. Only makes sense for client-streaming RPCs.
    /// @param request where to put the request on success
    /// @returns `true` on success, `false` on end-of-input
    /// @throws ugrpc::server::RpcError on an RPC error
    /// @throws ugrpc::server::impl::MiddlewareRpcInterruptionError on error from middlewares
    bool DoRead(Request& request);

    /// @brief Write the next outgoing message. Only makes sense for server-streaming RPCs.
    /// @param response the next message to write
    /// @throws ugrpc::server::RpcError on an RPC error
    /// @throws ugrpc::server::impl::MiddlewareRpcInterruptionError on error from middlewares
    void DoWrite(Response& response);

    /// @brief Complete the RPC with an error
    ///
    /// `Finish` must not be called multiple times.
    ///
    /// @param status error details
    /// @throws ugrpc::server::RpcError on an RPC error
    void FinishWithError(const grpc::Status& status);

    /// @brief Complete the RPC successfully, sending the given response message to the client.
    ///
    /// For response-streaming calls, this is mostly equivalent to `Write + Finish`, but saves one round-trip.
    ///
    /// `Finish` must not be called multiple times for the same RPC.
    ///
    /// @param response the final `Response` message to send to the client
    /// @throws ugrpc::server::RpcError on an RPC error
    void Finish(const Response& response);

    /// @brief Complete the RPC with `OK` status, without a final response. Only makes sense for server-streaming RPCs.
    ///
    /// `Finish` must not be called multiple times.
    ///
    /// @throws ugrpc::server::RpcError on an RPC error
    void Finish();

private:
    RawCall& stream_;
    // Separate flags are required to be able to set them in parallel in Read and Write.
    bool are_reads_done_{kCallKind == CallKind::kUnaryCall};
    bool is_finished_{false};
};

template <typename CallTraits>
Call<CallTraits>::Call(CallParams&& call_params, RawCall& stream)
    : CallAnyBase(utils::impl::InternalTag{}, std::move(call_params), kCallKind), stream_(stream) {}

template <typename CallTraits>
Call<CallTraits>::~Call() {
    if (!is_finished_) {
        if constexpr (kCallKind == CallKind::kUnaryCall || kCallKind == CallKind::kInputStream) {
            impl::CancelWithError(stream_, GetCallName());
        } else {
            impl::Cancel(stream_, GetCallName());
        }
    }
}

template <typename CallTraits>
bool Call<CallTraits>::DoRead(Request& request) {
    static_assert(kCallKind == CallKind::kInputStream || kCallKind == CallKind::kBidirectionalStream);
    UINVARIANT(!are_reads_done_, "'Read' called while the stream is half-closed for reads");
    if (impl::Read(stream_, request)) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, Request>) {
            ApplyRequestHook(&request);
        }
        return true;
    } else {
        are_reads_done_ = true;
        return false;
    }
}

template <typename CallTraits>
void Call<CallTraits>::DoWrite(Response& response) {
    static_assert(kCallKind == CallKind::kOutputStream || kCallKind == CallKind::kBidirectionalStream);
    UINVARIANT(!is_finished_, "'Write' called on a finished stream");
    if constexpr (std::is_base_of_v<google::protobuf::Message, Response>) {
        ApplyResponseHook(&response);
    }

    if constexpr (kCallKind == CallKind::kOutputStream) {
        // For some reason, gRPC requires explicit 'SendInitialMetadata' in output streams.
        if (!are_reads_done_) {
            are_reads_done_ = true;
            SendInitialMetadata(stream_, GetCallName());
        }
    }

    // Don't buffer writes, otherwise in an event subscription scenario, events
    // may never actually be delivered.
    grpc::WriteOptions write_options{};

    try {
        impl::Write(stream_, response, write_options, GetCallName());
    } catch (const RpcInterruptedError&) {
        is_finished_ = true;
        throw;
    }
}

template <typename CallTraits>
void Call<CallTraits>::FinishWithError(const grpc::Status& status) {
    UASSERT(!status.ok());
    UINVARIANT(!is_finished_, "'FinishWithError' called on a finished stream");
    is_finished_ = true;

    if constexpr (kCallKind == CallKind::kUnaryCall || kCallKind == CallKind::kInputStream) {
        impl::FinishWithError(stream_, status, GetCallName());
    } else {
        impl::Finish(stream_, status, GetCallName());
    }

    PostFinish(status);
}

template <typename CallTraits>
void Call<CallTraits>::Finish(const Response& response) {
    UINVARIANT(!is_finished_, "'Finish' called on a finished stream");
    is_finished_ = true;

    if constexpr (kCallKind == CallKind::kUnaryCall || kCallKind == CallKind::kInputStream) {
        impl::Finish(stream_, response, grpc::Status::OK, GetCallName());
    } else {
        // Don't buffer writes, optimize for ping-pong-style interaction.
        grpc::WriteOptions write_options{};

        impl::WriteAndFinish(stream_, response, write_options, grpc::Status::OK, GetCallName());
    }

    PostFinish(grpc::Status::OK);
}

template <typename CallTraits>
void Call<CallTraits>::Finish() {
    UINVARIANT(!is_finished_, "'Finish' called on a finished stream");
    is_finished_ = true;

    impl::Finish(stream_, grpc::Status::OK, GetCallName());

    PostFinish(grpc::Status::OK);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
