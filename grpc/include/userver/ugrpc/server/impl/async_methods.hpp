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

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Finish(GrpcStream& stream, const Response& response, const grpc::Status& status) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Finish(response, status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool Finish(GrpcStream& stream, const grpc::Status& status) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Finish(status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool FinishWithError(GrpcStream& stream, const grpc::Status& status) {
    UASSERT(!status.ok());
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.FinishWithError(status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream>
[[nodiscard]] bool SendInitialMetadata(GrpcStream& stream) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.SendInitialMetadata(invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream, typename Request>
[[nodiscard]] bool Read(GrpcStream& stream, Request& request) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Read(&request, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Write(GrpcStream& stream, const Response& response, grpc::WriteOptions options) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.Write(response, options, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool WriteAndFinish(
    GrpcStream& stream,
    const Response& response,
    grpc::WriteOptions options,
    const grpc::Status& status
) {
    ugrpc::impl::AsyncMethodInvocation invocation;
    stream.WriteAndFinish(response, options, status, invocation.GetCompletionTag());
    return invocation.WaitNonCancellable();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
