#pragma once

#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_method_invocation.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void CheckOk(StreamingCallState& state, ugrpc::impl::AsyncMethodInvocation::WaitStatus status, std::string_view stage);

template <typename GrpcStream>
void StartCall(GrpcStream& stream, StreamingCallState& state) {
    ugrpc::impl::AsyncMethodInvocation start_call;
    stream.StartCall(start_call.GetCompletionTag());
    CheckOk(state, WaitAndTryCancelIfNeeded(start_call, state.GetClientContext()), "StartCall");
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

    FinishAsyncMethodInvocation finish;
    auto& status = state.GetStatus();
    stream.Finish(&status, finish.GetCompletionTag());

    const auto wait_status = WaitAndTryCancelIfNeeded(finish, state.GetClientContext());
    switch (wait_status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            state.GetStatsScope().SetFinishTime(finish.GetFinishTime());
            try {
                ProcessFinish(state, final_response);
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
            state.GetStatsScope().SetFinishTime(finish.GetFinishTime());
            ProcessNetworkError(state, "Finish");
            if (throw_on_error) {
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
void FinishAbandoned(GrpcStream& stream, StreamingCallState& state) noexcept try {
    if (state.IsFinished()) {
        return;
    }
    state.SetFinished();

    state.GetClientContext().TryCancel();

    FinishAsyncMethodInvocation finish;
    stream.Finish(&state.GetStatus(), finish.GetCompletionTag());

    const engine::TaskCancellationBlocker cancel_blocker;
    const auto wait_status = finish.Wait();

    state.GetStatsScope().SetFinishTime(finish.GetFinishTime());

    switch (wait_status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            ProcessFinishAbandoned(state);
            break;
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
            ProcessNetworkError(state, "Finish");
            break;
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            UINVARIANT(false, "unreachable");
    }
} catch (const std::exception& ex) {
    LOG_WARNING() << "There is a caught exception in 'FinishAbandoned': " << ex;
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response, StreamingCallState& state) {
    UINVARIANT(IsReadAvailable(state), "'impl::Read' called on a finished call");
    ugrpc::impl::AsyncMethodInvocation read;
    stream.Read(&response, read.GetCompletionTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(read, state.GetClientContext());
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    return wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Response>
void ReadAsync(GrpcStream& stream, Response& response, StreamingCallState& state) {
    UINVARIANT(IsReadAvailable(state), "'impl::Read' called on a finished call");
    state.EmplaceAsyncMethodInvocation();
    auto& read = state.GetAsyncMethodInvocation();
    stream.Read(&response, read.GetCompletionTag());
}

template <typename GrpcStream, typename Request>
bool Write(GrpcStream& stream, const Request& request, grpc::WriteOptions options, StreamingCallState& state) {
    const auto lock = state.TakeMutexIfBidirectional();
    if (state.IsFinished()) {
        // It't forbidden to work with a stream after Finish.
        throw ugrpc::client::RpcInterruptedError{state.GetCallName(), "Write"};
    }

    UINVARIANT(IsWriteAvailable(state), "'impl::Write' called on a stream that is closed for writes");

    ugrpc::impl::AsyncMethodInvocation write;
    stream.Write(request, options, write.GetCompletionTag());
    const auto result = WaitAndTryCancelIfNeeded(write, state.GetClientContext());
    if (result == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    if (result != ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk) {
        state.SetWritesFinished();
    }
    return result == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Request>
void WriteAndCheck(GrpcStream& stream, const Request& request, grpc::WriteOptions options, StreamingCallState& state) {
    const auto lock = state.TakeMutexIfBidirectional();
    if (state.IsFinished()) {
        // It't forbidden to work with a stream after Finish.
        throw ugrpc::client::RpcInterruptedError{state.GetCallName(), "WriteAndCheck"};
    }

    UINVARIANT(IsWriteAndCheckAvailable(state), "'impl::WriteAndCheck' called on a finished or closed stream");

    ugrpc::impl::AsyncMethodInvocation write;
    stream.Write(request, options, write.GetCompletionTag());
    CheckOk(state, WaitAndTryCancelIfNeeded(write, state.GetClientContext()), "WriteAndCheck");
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
    ugrpc::impl::AsyncMethodInvocation writes_done;
    stream.WritesDone(writes_done.GetCompletionTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(writes_done, state.GetClientContext());
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    return wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
