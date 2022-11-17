#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/status.h>

#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
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

enum class State { kOpen, kWritesDone, kFinished };

class RpcData final {
 public:
  RpcData(std::unique_ptr<grpc::ClientContext>&& context,
          std::string_view call_name,
          ugrpc::impl::MethodStatistics& statistics);

  RpcData(RpcData&&) noexcept = delete;
  RpcData& operator=(RpcData&&) noexcept = delete;
  ~RpcData();

  const grpc::ClientContext& GetContext() const noexcept;

  grpc::ClientContext& GetContext() noexcept;

  std::string_view GetCallName() const noexcept;

  tracing::Span& GetSpan() noexcept;

  void ResetSpan() noexcept;

  ugrpc::impl::RpcStatisticsScope& GetStatsScope() noexcept;

  State GetState() const noexcept;

  void SetState(State new_state) noexcept;

  void EmplaceAsyncMethodInvocation();

  AsyncMethodInvocation& GetAsyncMethodInvocation() noexcept;

  grpc::Status& GetStatus() noexcept;

  class AsyncMethodInvocationGuard {
   public:
    AsyncMethodInvocationGuard(RpcData& data) noexcept;
    ~AsyncMethodInvocationGuard() noexcept;

   private:
    RpcData& data_;
  };

 private:
  std::unique_ptr<grpc::ClientContext> context_;
  std::string_view call_name_;
  State state_{State::kOpen};

  std::optional<tracing::InPlaceSpan> span_;
  ugrpc::impl::RpcStatisticsScope stats_scope_;

  std::optional<AsyncMethodInvocation> finish_;
  grpc::Status status_;
};

void CheckOk(RpcData& data, bool ok, std::string_view stage);

template <typename GrpcStream>
void StartCall(GrpcStream& stream, RpcData& data) {
  AsyncMethodInvocation start_call;
  stream.StartCall(start_call.GetTag());
  CheckOk(data, start_call.Wait(), "StartCall");
}

void PrepareFinish(RpcData& data);

void ProcessFinishResult(RpcData& data, bool ok, grpc::Status& status,
                         const std::optional<std::string>& message);

void ProcessFinishResultAndCheck(RpcData& data, bool ok, grpc::Status& status);

template <typename GrpcStream>
void Finish(GrpcStream& stream, RpcData& data) {
  PrepareFinish(data);
  grpc::Status status;
  AsyncMethodInvocation finish;
  stream.Finish(&status, finish.GetTag());
  ProcessFinishResultAndCheck(data, finish.Wait(), status);
}

void PrepareRead(RpcData& data);

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response, RpcData& data) {
  PrepareRead(data);
  AsyncMethodInvocation read;
  stream.Read(&response, read.GetTag());
  return read.Wait();
}

void PrepareWrite(RpcData& data);

template <typename GrpcStream, typename Request>
void Write(GrpcStream& stream, const Request& request,
           grpc::WriteOptions options, RpcData& data) {
  PrepareWrite(data);
  AsyncMethodInvocation write;
  stream.Write(request, options, write.GetTag());
  CheckOk(data, write.Wait(), "Write");
}

template <typename GrpcStream>
void WritesDone(GrpcStream& stream, RpcData& data) {
  PrepareWrite(data);
  data.SetState(State::kWritesDone);
  AsyncMethodInvocation writes_done;
  stream.WritesDone(writes_done.GetTag());
  CheckOk(data, writes_done.Wait(), "WritesDone");
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
