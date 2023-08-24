#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/status.h>

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

void ReportErrorWhileCancelling(std::string_view call_name) noexcept;

void ThrowOnError(impl::AsyncMethodInvocation::WaitStatus status,
                  std::string_view call_name, std::string_view stage_name);

extern const grpc::Status kUnimplementedStatus;
extern const grpc::Status kUnknownErrorStatus;

template <typename GrpcStream, typename Response>
void Finish(GrpcStream& stream, const Response& response,
            const grpc::Status& status, std::string_view call_name) {
  AsyncMethodInvocation finish;
  stream.Finish(response, status, finish.GetTag());
  ThrowOnError(Wait(finish), call_name, "Finish");
}

template <typename GrpcStream>
void Finish(GrpcStream& stream, const grpc::Status& status,
            std::string_view call_name) {
  AsyncMethodInvocation finish;
  stream.Finish(status, finish.GetTag());
  ThrowOnError(Wait(finish), call_name, "Finish");
}

template <typename GrpcStream>
void Cancel(GrpcStream& stream, std::string_view call_name) noexcept {
  AsyncMethodInvocation cancel;
  stream.Finish(kUnknownErrorStatus, cancel.GetTag());
  if (Wait(cancel) != impl::AsyncMethodInvocation::WaitStatus::kOk)
    ReportErrorWhileCancelling(call_name);
}

template <typename GrpcStream>
void CancelWithError(GrpcStream& stream, std::string_view call_name) noexcept {
  AsyncMethodInvocation cancel;
  stream.FinishWithError(kUnknownErrorStatus, cancel.GetTag());
  if (Wait(cancel) != impl::AsyncMethodInvocation::WaitStatus::kOk)
    ReportErrorWhileCancelling(call_name);
}

template <typename GrpcStream>
void FinishWithError(GrpcStream& stream, const grpc::Status& status,
                     std::string_view call_name) {
  AsyncMethodInvocation finish;
  stream.FinishWithError(status, finish.GetTag());
  ThrowOnError(Wait(finish), call_name, "FinishWithError");
}

template <typename GrpcStream>
void SendInitialMetadata(GrpcStream& stream, std::string_view call_name) {
  AsyncMethodInvocation metadata;
  stream.SendInitialMetadata(metadata.GetTag());
  ThrowOnError(Wait(metadata), call_name, "SendInitialMetadata");
}

template <typename GrpcStream, typename Request>
bool Read(GrpcStream& stream, Request& request) {
  AsyncMethodInvocation read;
  stream.Read(&request, read.GetTag());
  return Wait(read) == impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename GrpcStream, typename Response>
void Write(GrpcStream& stream, const Response& response,
           grpc::WriteOptions options, std::string_view call_name) {
  AsyncMethodInvocation write;
  stream.Write(response, options, write.GetTag());
  ThrowOnError(Wait(write), call_name, "Write");
}

template <typename GrpcStream, typename Response>
void WriteAndFinish(GrpcStream& stream, const Response& response,
                    grpc::WriteOptions options, const grpc::Status& status,
                    std::string_view call_name) {
  AsyncMethodInvocation write_and_finish;
  stream.WriteAndFinish(response, options, status, write_and_finish.GetTag());
  ThrowOnError(Wait(write_and_finish), call_name, "WriteAndFinish");
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
