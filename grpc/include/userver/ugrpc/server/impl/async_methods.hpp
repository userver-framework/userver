#pragma once

#include <string_view>

#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/status.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

extern const grpc::Status kUnimplementedStatus;
extern const grpc::Status kUnknownErrorStatus;

void CheckInvocationSucceeded(bool ok, std::string_view call_name, std::string_view stage);

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Finish(GrpcStream& stream, const Response& response, const grpc::Status& status) {
    ugrpc::impl::AsyncMethodInvocation finish;
    stream.Finish(response, status, finish.GetCompletionTag());
    return finish.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool Finish(GrpcStream& stream, const grpc::Status& status) {
    ugrpc::impl::AsyncMethodInvocation finish;
    stream.Finish(status, finish.GetCompletionTag());
    return finish.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool FinishWithError(GrpcStream& stream, const grpc::Status& status) {
    UASSERT(!status.ok());
    ugrpc::impl::AsyncMethodInvocation finish;
    stream.FinishWithError(status, finish.GetCompletionTag());
    return finish.WaitNonCancellable();
}

template <typename GrpcStream>
void SendInitialMetadata(GrpcStream& stream, std::string_view call_name) {
    ugrpc::impl::AsyncMethodInvocation metadata;
    stream.SendInitialMetadata(metadata.GetCompletionTag());
    CheckInvocationSucceeded(metadata.WaitNonCancellable(), call_name, "SendInitialMetadata");
}

template <typename GrpcStream, typename Request>
[[nodiscard]] bool Read(GrpcStream& stream, Request& request) {
    ugrpc::impl::AsyncMethodInvocation read;
    stream.Read(&request, read.GetCompletionTag());
    return read.WaitNonCancellable();
}

template <typename GrpcStream, typename Response>
void Write(GrpcStream& stream, const Response& response, grpc::WriteOptions options, std::string_view call_name) {
    ugrpc::impl::AsyncMethodInvocation write;
    stream.Write(response, options, write.GetCompletionTag());
    CheckInvocationSucceeded(write.WaitNonCancellable(), call_name, "Write");
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool WriteAndFinish(
    GrpcStream& stream,
    const Response& response,
    grpc::WriteOptions options,
    const grpc::Status& status
) {
    ugrpc::impl::AsyncMethodInvocation write_and_finish;
    stream.WriteAndFinish(response, options, status, write_and_finish.GetCompletionTag());
    return write_and_finish.WaitNonCancellable();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
