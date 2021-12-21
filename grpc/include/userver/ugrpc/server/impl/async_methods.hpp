#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/status.h>

#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/server/exceptions.hpp>

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

void ReportErrorWhileCancelling(std::string_view call_name) noexcept;

extern const grpc::Status kUnimplementedStatus;
extern const grpc::Status kUnknownErrorStatus;

template <typename GrpcStream, typename Response>
void Finish(GrpcStream& stream, const Response& response,
            const grpc::Status& status, std::string_view call_name) {
  AsyncMethodInvocation finish;
  stream.Finish(response, status, finish.GetTag());
  if (!finish.Wait()) {
    throw RpcInterruptedError(call_name, "Finish");
  }
}

template <typename GrpcStream>
void Finish(GrpcStream& stream, const grpc::Status& status,
            std::string_view call_name) {
  AsyncMethodInvocation finish;
  stream.Finish(status, finish.GetTag());
  if (!finish.Wait()) {
    throw RpcInterruptedError(call_name, "Finish");
  }
}

template <typename GrpcStream>
void Cancel(GrpcStream& stream, std::string_view call_name) noexcept {
  AsyncMethodInvocation cancel;
  stream.Finish(kUnknownErrorStatus, cancel.GetTag());
  if (!cancel.Wait()) ReportErrorWhileCancelling(call_name);
}

template <typename GrpcStream>
void CancelWithError(GrpcStream& stream, std::string_view call_name) noexcept {
  AsyncMethodInvocation cancel;
  stream.FinishWithError(kUnknownErrorStatus, cancel.GetTag());
  if (!cancel.Wait()) ReportErrorWhileCancelling(call_name);
}

template <typename GrpcStream>
void FinishWithError(GrpcStream& stream, const grpc::Status& status,
                     std::string_view call_name) {
  AsyncMethodInvocation finish;
  stream.FinishWithError(status, finish.GetTag());
  if (!finish.Wait()) {
    throw RpcInterruptedError(call_name, "FinishWithError");
  }
}

template <typename GrpcStream>
void SendInitialMetadata(GrpcStream& stream, std::string_view call_name) {
  AsyncMethodInvocation metadata;
  stream.SendInitialMetadata(metadata.GetTag());
  if (!metadata.Wait()) {
    throw RpcInterruptedError(call_name, "SendInitialMetadata");
  }
}

template <typename GrpcStream, typename Request>
bool Read(GrpcStream& stream, Request& request) {
  AsyncMethodInvocation read;
  stream.Read(&request, read.GetTag());
  return read.Wait();
}

template <typename GrpcStream, typename Response>
void Write(GrpcStream& stream, const Response& response,
           grpc::WriteOptions options, std::string_view call_name) {
  AsyncMethodInvocation write;
  stream.Write(response, options, write.GetTag());
  if (!write.Wait()) {
    throw RpcInterruptedError(call_name, "Write");
  }
}

template <typename GrpcStream, typename Response>
void WriteAndFinish(GrpcStream& stream, const Response& response,
                    grpc::WriteOptions options, const grpc::Status& status,
                    std::string_view call_name) {
  AsyncMethodInvocation write_and_finish;
  stream.WriteAndFinish(response, options, status, write_and_finish.GetTag());
  if (!write_and_finish.Wait()) {
    throw RpcInterruptedError(call_name, "WriteAndFinish");
  }
}

template <typename GrpcStream, typename State>
void SendInitialMetadataIfNew(GrpcStream& stream, std::string_view call_name,
                              State& state) {
  if (state == State::kNew) {
    state = State::kOpen;
    SendInitialMetadata(stream, call_name);
  }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
