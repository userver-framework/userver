#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <google/protobuf/message.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/status.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_method_invocation.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// @{
/// @brief Helper type aliases for low-level asynchronous gRPC streams
/// @see <grpcpp/impl/codegen/async_unary_call_impl.h>
/// @see <grpcpp/impl/codegen/async_stream_impl.h>
template <typename Response>
using RawResponseReader = std::unique_ptr<grpc::ClientAsyncResponseReader<Response>>;

template <typename Response>
using RawReader = std::unique_ptr<grpc::ClientAsyncReader<Response>>;

template <typename Request>
using RawWriter = std::unique_ptr<grpc::ClientAsyncWriter<Request>>;

template <typename Request, typename Response>
using RawReaderWriter = std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>>;
/// @}

template <typename Message>
const google::protobuf::Message* ToBaseMessage(const Message& message) {
    if constexpr (std::is_base_of_v<google::protobuf::Message, Message>) {
        return &message;
    } else {
        return nullptr;
    }
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitAndTryCancelIfNeeded(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    engine::Deadline deadline,
    grpc::ClientContext& context
) noexcept;

ugrpc::impl::AsyncMethodInvocation::WaitStatus
WaitAndTryCancelIfNeeded(ugrpc::impl::AsyncMethodInvocation& invocation, grpc::ClientContext& context) noexcept;

void CheckOk(CallState& state, AsyncMethodInvocation::WaitStatus status, std::string_view stage);

template <typename GrpcStream>
void StartCall(GrpcStream& stream, CallState& state) {
    AsyncMethodInvocation start_call;
    stream.StartCall(start_call.GetTag());
    CheckOk(state, WaitAndTryCancelIfNeeded(start_call, state.GetContext()), "StartCall");
}

void PrepareFinish(CallState& state);

void ProcessFinish(CallState& state, const google::protobuf::Message* final_response);

void ProcessFinishCancelled(CallState& state);

void CheckFinishStatus(CallState& state);

template <typename GrpcStream>
void Finish(
    GrpcStream& stream,
    CallState& state,
    const google::protobuf::Message* final_response,
    bool throw_on_error
) {
    PrepareFinish(state);

    FinishAsyncMethodInvocation finish;
    auto& status = state.GetStatus();
    stream.Finish(&status, finish.GetTag());

    const auto wait_status = WaitAndTryCancelIfNeeded(finish, state.GetContext());
    if (wait_status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        ProcessFinishCancelled(state);
        // Finish AsyncMethodInvocation will be awaited in its destructor.
        if (throw_on_error) {
            throw RpcCancelledError(state.GetCallName(), "Finish");
        }
    }

    UINVARIANT(
        impl::AsyncMethodInvocation::WaitStatus::kOk == wait_status, "Client-side Finish: ok should always be true"
    );
    state.GetStatsScope().SetFinishTime(finish.GetFinishTime());
    ProcessFinish(state, final_response);
    if (throw_on_error) {
        CheckFinishStatus(state);
    }
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response, CallState& state) {
    UINVARIANT(state.IsReadAvailable(), "'impl::Read' called on a finished call");
    AsyncMethodInvocation read;
    stream.Read(&response, read.GetTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(read, state.GetContext());
    if (wait_status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    return wait_status == impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Response>
void ReadAsync(GrpcStream& stream, Response& response, CallState& state) {
    UINVARIANT(state.IsReadAvailable(), "'impl::Read' called on a finished call");
    state.EmplaceAsyncMethodInvocation();
    auto& read = state.GetAsyncMethodInvocation();
    stream.Read(&response, read.GetTag());
}

template <typename GrpcStream, typename Request>
bool Write(GrpcStream& stream, const Request& request, grpc::WriteOptions options, CallState& state) {
    UINVARIANT(state.IsWriteAvailable(), "'impl::Write' called on a stream that is closed for writes");
    AsyncMethodInvocation write;
    stream.Write(request, options, write.GetTag());
    const auto result = WaitAndTryCancelIfNeeded(write, state.GetContext());
    if (result == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    if (result != impl::AsyncMethodInvocation::WaitStatus::kOk) {
        state.SetWritesFinished();
    }
    return result == impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Request>
void WriteAndCheck(GrpcStream& stream, const Request& request, grpc::WriteOptions options, CallState& state) {
    UINVARIANT(state.IsWriteAndCheckAvailable(), "'impl::WriteAndCheck' called on a finished or closed stream");
    AsyncMethodInvocation write;
    stream.Write(request, options, write.GetTag());
    CheckOk(state, WaitAndTryCancelIfNeeded(write, state.GetContext()), "WriteAndCheck");
}

template <typename GrpcStream>
bool WritesDone(GrpcStream& stream, CallState& state) {
    UINVARIANT(state.IsWriteAvailable(), "'impl::WritesDone' called on a stream that is closed for writes");
    state.SetWritesFinished();
    AsyncMethodInvocation writes_done;
    stream.WritesDone(writes_done.GetTag());
    const auto wait_status = WaitAndTryCancelIfNeeded(writes_done, state.GetContext());
    if (wait_status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
    }
    return wait_status == impl::AsyncMethodInvocation::WaitStatus::kOk;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
