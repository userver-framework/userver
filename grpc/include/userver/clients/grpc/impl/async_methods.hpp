#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/status.h>

#include <userver/clients/grpc/errors.hpp>
#include <userver/engine/single_use_event.hpp>

namespace clients::grpc::impl {

/// @{
/// @brief Helper type aliases for gRPC streams
/// @see <grpcpp/impl/codegen/async_unary_call_impl.h>
/// @see <grpcpp/impl/codegen/async_stream_impl.h>
template <typename Response>
using AsyncResponseReader =
    std::unique_ptr<::grpc::ClientAsyncResponseReader<Response>>;

template <typename Response>
using AsyncInputStream = std::unique_ptr<::grpc::ClientAsyncReader<Response>>;

template <typename Request>
using AsyncOutputStream = std::unique_ptr<::grpc::ClientAsyncWriter<Request>>;

template <typename Request, typename Response>
using AsyncBidirectionalStream =
    std::unique_ptr<::grpc::ClientAsyncReaderWriter<Request, Response>>;
/// @}

/// @{
/// @brief Helper type aliases for stub member function pointers
template <typename Stub, typename Request, typename Response>
using PrepareUnaryCallFunc = AsyncResponseReader<Response> (Stub::*)(
    ::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using PrepareInputStreamFunc = AsyncInputStream<Response> (Stub::*)(
    ::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using PrepareOutputStreamFunc = AsyncOutputStream<Request> (Stub::*)(
    ::grpc::ClientContext*, Response*, ::grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using PrepareBidirectionalStreamFunc =
    AsyncBidirectionalStream<Request, Response> (Stub::*)(
        ::grpc::ClientContext*, ::grpc::CompletionQueue*);
/// @}

class AsyncMethodInvocation final {
 public:
  constexpr AsyncMethodInvocation() noexcept = default;

  /// For use from the blocking call queue
  void Notify(bool ok) noexcept;

  /// For use from coroutines
  void* GetTag() noexcept;
  [[nodiscard]] bool Wait() noexcept;

 private:
  bool ok_{false};
  engine::SingleUseEvent event_;
};

void ProcessStartCallResult(std::string_view call_name, bool ok);

template <typename GrpcStream>
void StartCall(GrpcStream& stream, std::string_view call_name) {
  AsyncMethodInvocation start_call;
  stream.StartCall(start_call.GetTag());
  ProcessStartCallResult(call_name, start_call.Wait());
}

void ProcessFinishResult(std::string_view call_name, bool ok,
                         ::grpc::Status&& status);

template <typename GrpcStream>
void Finish(GrpcStream& stream, std::string_view call_name) {
  ::grpc::Status status;
  AsyncMethodInvocation finish;
  stream.Finish(&status, finish.GetTag());
  ProcessFinishResult(call_name, finish.Wait(), std::move(status));
}

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response) {
  impl::AsyncMethodInvocation read;
  stream.Read(&response, read.GetTag());
  return read.Wait();
}

template <typename GrpcStream, typename Request>
[[nodiscard]] bool Write(GrpcStream& stream, const Request& request,
                         ::grpc::WriteOptions options) {
  impl::AsyncMethodInvocation write;
  stream.Write(request, options, write.GetTag());
  return write.Wait();
}

template <typename GrpcStream>
[[nodiscard]] bool WritesDone(GrpcStream& stream) {
  AsyncMethodInvocation writes_done;
  stream.WritesDone(writes_done.GetTag());
  return writes_done.Wait();
}

}  // namespace clients::grpc::impl
