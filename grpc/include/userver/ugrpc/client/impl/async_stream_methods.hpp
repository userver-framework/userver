#pragma once

#include <string_view>

#include <grpcpp/support/async_stream.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_method_invocation.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/impl/status_utils.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitAndTryCancelIfNeeded(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& client_context
) noexcept;

void CheckOk(
    StreamingCallState& state,
    ugrpc::impl::AsyncMethodInvocation::WaitStatus wait_status,
    std::string_view stage
);

void CheckFinishStatus(CallState& state);

void ProcessFinish(CallState& state, const grpc::Status& status, const google::protobuf::Message* final_response);

void ProcessFinishAbandoned(CallState& state, const grpc::Status& status) noexcept;

void ProcessCancelled(CallState& state, std::string_view stage) noexcept;

void ProcessNetworkError(CallState& state, std::string_view stage) noexcept;

void ThrowIfDeadlineIsExceeded(grpc::ClientContext& client_context, std::string_view call_name);

template <typename GrpcStream>
void StartCall(GrpcStream& stream, StreamingCallState& state) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.StartCall(invocation.GetCompletionTag());
    CheckOk(state, WaitAndTryCancelIfNeeded(invocation, state.GetClientContext()), "StartCall");
}

template <typename GrpcStream>
void Finish(
    GrpcStream& stream,
    StreamingCallState& state,
    const google::protobuf::Message* final_response,
    bool throw_on_error
) {
    const auto lock = state.TakeMutexIfBidirectional();

    state.SetFinished();

    FinishAsyncMethodInvocation invocation;
    stream.Finish(&state.GetStatus(), invocation.GetCompletionTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(invocation, state.GetClientContext());

    switch (wait_status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            state.GetStatsScope().SetFinishTime(invocation.GetNotifyTime());
            try {
                ugrpc::impl::ClampStatusCodeToValidRange(state.GetStatus());
                ProcessFinish(state, state.GetStatus(), final_response);
            } catch (const std::exception& ex) {
                if (throw_on_error) {
                    throw;
                } else {
                    LOG_WARNING() << "There is a caught exception in 'impl::Finish': " << ex;
                }
            }
            if (throw_on_error) {
                CheckFinishStatus(state);
            }
            break;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
            state.GetStatsScope().SetFinishTime(invocation.GetNotifyTime());
            ProcessNetworkError(state, "Finish");
            if (throw_on_error) {
                ThrowIfDeadlineIsExceeded(state.GetClientContext(), state.GetCallName());
                throw RpcInterruptedError(state.GetCallName(), "Finish");
            }
            break;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            ProcessCancelled(state, "Finish");
            // Finish AsyncMethodInvocation will be awaited in its destructor.
            if (throw_on_error) {
                throw RpcCancelledError(state.GetCallName(), "Finish");
            }
            break;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            UINVARIANT(false, "unreachable");
    }
}

template <typename GrpcStream>
void FinishAbandoned(GrpcStream& stream, StreamingCallState& state) noexcept try
{
    if (state.IsFinished()) {
        return;
    }
    state.SetFinished();

    state.GetClientContext().TryCancel();

    FinishAsyncMethodInvocation invocation;
    stream.Finish(&state.GetStatus(), invocation.GetCompletionTag());
    const auto ok = invocation.WaitNonCancellable();

    state.GetStatsScope().SetFinishTime(invocation.GetNotifyTime());

    if (ok) {
        ugrpc::impl::ClampStatusCodeToValidRange(state.GetStatus());
        ProcessFinishAbandoned(state, state.GetStatus());
    } else {
        ProcessNetworkError(state, "Finish");
    }
} catch (const std::exception& ex) {
    LOG_WARNING() << "There is a caught exception in 'FinishAbandoned': " << ex;
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response, StreamingCallState& state) {
    UINVARIANT(IsReadAvailable(state), "'impl::Read' called on a finished call");
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Read(&response, invocation.GetCompletionTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(invocation, state.GetClientContext());
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    return wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Response>
void ReadAsync(GrpcStream& stream, Response& response, StreamingCallState& state) {
    UINVARIANT(IsReadAvailable(state), "'impl::Read' called on a finished call");
    state.EmplaceAsyncMethodInvocation();
    auto& invocation = state.GetAsyncMethodInvocation();
    stream.Read(&response, invocation.GetCompletionTag());
}

template <typename GrpcStream, typename Request>
bool Write(GrpcStream& stream, const Request& request, grpc::WriteOptions options, StreamingCallState& state) {
    const auto lock = state.TakeMutexIfBidirectional();
    if (state.IsFinished()) {
        // It't forbidden to work with a stream after Finish.
        return false;
    }

    UINVARIANT(IsWriteAvailable(state), "'impl::Write' called on a stream that is closed for writes");

    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Write(request, options, invocation.GetCompletionTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(invocation, state.GetClientContext());
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    if (wait_status != ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk) {
        state.SetWritesFinished();
    }
    return wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Request>
void WriteAndCheck(GrpcStream& stream, const Request& request, grpc::WriteOptions options, StreamingCallState& state) {
    const auto lock = state.TakeMutexIfBidirectional();
    if (state.IsFinished()) {
        // It't forbidden to work with a stream after Finish.
        throw ugrpc::client::RpcInterruptedError{state.GetCallName(), "WriteAndCheck"};
    }

    UINVARIANT(IsWriteAndCheckAvailable(state), "'impl::WriteAndCheck' called on a finished or closed stream");

    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Write(request, options, invocation.GetCompletionTag());
    CheckOk(state, WaitAndTryCancelIfNeeded(invocation, state.GetClientContext()), "WriteAndCheck");
}

template <typename GrpcStream>
bool WritesDone(GrpcStream& stream, StreamingCallState& state) {
    const auto lock = state.TakeMutexIfBidirectional();
    if (state.IsFinished()) {
        // It't forbidden to work with a stream after Finish.
        throw ugrpc::client::RpcInterruptedError{state.GetCallName(), "WritesDone"};
    }

    UINVARIANT(IsWriteAvailable(state), "'impl::WritesDone' called on a stream that is closed for writes");
    state.SetWritesFinished();
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.WritesDone(invocation.GetCompletionTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(invocation, state.GetClientContext());
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    return wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
