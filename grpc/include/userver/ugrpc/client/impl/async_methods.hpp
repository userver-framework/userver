#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/status.h>

#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// @{
/// @brief Helper type aliases for low-level asynchronous gRPC streams
/// @see <grpcpp/impl/codegen/async_unary_call_impl.h>
/// @see <grpcpp/impl/codegen/async_stream_impl.h>
template <typename Response>
using RawResponseReader =
    std::unique_ptr<grpc::ClientAsyncResponseReader<Response>>;

template <typename Response>
using RawReader = std::unique_ptr<grpc::ClientAsyncReader<Response>>;

template <typename Request>
using RawWriter = std::unique_ptr<grpc::ClientAsyncWriter<Request>>;

template <typename Request, typename Response>
using RawReaderWriter =
    std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>>;
/// @}

/// @{
/// @brief Helper type aliases for stub member function pointers
template <typename Stub, typename Request, typename Response>
using RawResponseReaderPreparer = RawResponseReader<Response> (Stub::*)(
    grpc::ClientContext*, const Request&, grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using RawReaderPreparer = RawReader<Response> (Stub::*)(grpc::ClientContext*,
                                                        const Request&,
                                                        grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using RawWriterPreparer = RawWriter<Request> (Stub::*)(grpc::ClientContext*,
                                                       Response*,
                                                       grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using RawReaderWriterPreparer = RawReaderWriter<Request, Response> (Stub::*)(
    grpc::ClientContext*, grpc::CompletionQueue*);
/// @}

using ugrpc::impl::AsyncMethodInvocation;

void ProcessStartCallResult(std::string_view call_name,
                            ugrpc::impl::RpcStatisticsScope& stats, bool ok);

template <typename GrpcStream>
void StartCall(GrpcStream& stream, std::string_view call_name,
               ugrpc::impl::RpcStatisticsScope& stats) {
  AsyncMethodInvocation start_call;
  stream.StartCall(start_call.GetTag());
  ProcessStartCallResult(call_name, stats, start_call.Wait());
}

void ProcessFinishResult(std::string_view call_name,
                         ugrpc::impl::RpcStatisticsScope& stats, bool ok,
                         grpc::Status&& status);

template <typename GrpcStream, typename Response>
void FinishUnary(GrpcStream& stream, Response& response, grpc::Status& status,
                 std::string_view call_name,
                 ugrpc::impl::RpcStatisticsScope& stats) {
  AsyncMethodInvocation finish_call;
  stream.Finish(&response, &status, finish_call.GetTag());
  ProcessFinishResult(call_name, stats, finish_call.Wait(), std::move(status));
}

template <typename GrpcStream>
void Finish(GrpcStream& stream, std::string_view call_name,
            ugrpc::impl::RpcStatisticsScope& stats) {
  grpc::Status status;
  AsyncMethodInvocation finish;
  stream.Finish(&status, finish.GetTag());
  ProcessFinishResult(call_name, stats, finish.Wait(), std::move(status));
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response) {
  AsyncMethodInvocation read;
  stream.Read(&response, read.GetTag());
  return read.Wait();
}

template <typename GrpcStream, typename Request>
[[nodiscard]] bool Write(GrpcStream& stream, const Request& request,
                         grpc::WriteOptions options) {
  AsyncMethodInvocation write;
  stream.Write(request, options, write.GetTag());
  return write.Wait();
}

template <typename GrpcStream>
[[nodiscard]] bool WritesDone(GrpcStream& stream) {
  AsyncMethodInvocation writes_done;
  stream.WritesDone(writes_done.GetTag());
  return writes_done.Wait();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
