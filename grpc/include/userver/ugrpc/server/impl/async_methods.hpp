#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/status.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @{
/// @brief Helper type aliases for low-level asynchronous gRPC streams
/// @see <grpcpp/impl/codegen/async_unary_call_impl.h>
/// @see <grpcpp/impl/codegen/async_stream_impl.h>
template <typename Response>
using RawResponseWriter = grpc::ServerAsyncResponseWriter<Response>;

template <typename Request, typename Response>
using RawReader = grpc::ServerAsyncReader<Response, Request>;

template <typename Response>
using RawWriter = grpc::ServerAsyncWriter<Response>;

template <typename Request, typename Response>
using RawReaderWriter = grpc::ServerAsyncReaderWriter<Response, Request>;
/// @}

using ugrpc::impl::AsyncMethodInvocation;

extern const grpc::Status kUnimplementedStatus;
extern const grpc::Status kUnknownErrorStatus;

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Finish(GrpcStream& stream, const Response& response, const grpc::Status& status) {
    AsyncMethodInvocation finish;
    stream.Finish(response, status, finish.GetCompletionTag());
    return IsInvocationSuccessful(Wait(finish));
}

template <typename GrpcStream>
[[nodiscard]] bool Finish(GrpcStream& stream, const grpc::Status& status) {
    AsyncMethodInvocation finish;
    stream.Finish(status, finish.GetCompletionTag());
    return IsInvocationSuccessful(Wait(finish));
}

template <typename GrpcStream>
[[nodiscard]] bool FinishWithError(GrpcStream& stream, const grpc::Status& status) {
    UASSERT(!status.ok());
    AsyncMethodInvocation finish;
    stream.FinishWithError(status, finish.GetCompletionTag());
    return IsInvocationSuccessful(Wait(finish));
}

template <typename GrpcStream>
void SendInitialMetadata(GrpcStream& stream, std::string_view call_name) {
    AsyncMethodInvocation metadata;
    stream.SendInitialMetadata(metadata.GetCompletionTag());
    CheckInvocationSuccessful(Wait(metadata), call_name, "SendInitialMetadata");
}

template <typename GrpcStream, typename Request>
[[nodiscard]] bool Read(GrpcStream& stream, Request& request) {
    AsyncMethodInvocation read;
    stream.Read(&request, read.GetCompletionTag());
    return Wait(read) == impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Response>
void Write(GrpcStream& stream, const Response& response, grpc::WriteOptions options, std::string_view call_name) {
    AsyncMethodInvocation write;
    stream.Write(response, options, write.GetCompletionTag());
    CheckInvocationSuccessful(Wait(write), call_name, "Write");
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool
WriteAndFinish(GrpcStream& stream, const Response& response, grpc::WriteOptions options, const grpc::Status& status) {
    AsyncMethodInvocation write_and_finish;
    stream.WriteAndFinish(response, options, status, write_and_finish.GetCompletionTag());
    return IsInvocationSuccessful(Wait(write_and_finish));
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
