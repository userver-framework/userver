#pragma once

#include <utility>

#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/status.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

extern const grpc::Status kUnimplementedStatus;
extern const grpc::Status kUnknownErrorStatus;

template <typename CallbackTraits>
class CallbackResponder;

template <typename GrpcStream>
[[nodiscard]] bool Finish(GrpcStream& stream, grpc::ByteBuffer&& response, const grpc::Status& status) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Finish(std::move(response), status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename CallbackTraits>
[[nodiscard]] bool Finish(
    CallbackResponder<CallbackTraits>& stream,
    grpc::ByteBuffer&& response,
    const grpc::Status& status
) {
    stream.Finish(std::move(response), status);
    return !stream.IsCancelled();
}

template <typename GrpcStream>
[[nodiscard]] bool Finish(GrpcStream& stream, const grpc::Status& status) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Finish(status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename CallbackTraits>
[[nodiscard]] bool Finish(CallbackResponder<CallbackTraits>& stream, const grpc::Status& status) {
    stream.Finish(status);
    return !stream.IsCancelled();
}

template <typename GrpcStream>
[[nodiscard]] bool FinishWithError(GrpcStream& stream, const grpc::Status& status) {
    UASSERT(!status.ok());
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.FinishWithError(status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename CallbackTraits>
[[nodiscard]] bool FinishWithError(CallbackResponder<CallbackTraits>& stream, const grpc::Status& status) {
    UASSERT(!status.ok());
    stream.FinishWithError(status);
    return !stream.IsCancelled();
}

template <typename GrpcStream>
[[nodiscard]] bool SendInitialMetadata(GrpcStream& stream) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.SendInitialMetadata(invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool Read(GrpcStream& stream, grpc::ByteBuffer& request) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Read(&request, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool Write(GrpcStream& stream, const grpc::ByteBuffer& response, grpc::WriteOptions options) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Write(response, options, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool WriteAndFinish(
    GrpcStream& stream,
    grpc::ByteBuffer&& response,
    grpc::WriteOptions options,
    const grpc::Status& status
) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.WriteAndFinish(std::move(response), options, status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename CallbackTraits>
[[nodiscard]] bool WriteAndFinish(
    CallbackResponder<CallbackTraits>& stream,
    grpc::ByteBuffer&& response,
    grpc::WriteOptions options,
    const grpc::Status& status
) {
    stream.WriteAndFinish(std::move(response), options, status);
    return !stream.IsCancelled();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
