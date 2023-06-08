#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/status.h>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/call_params.hpp>
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

struct RpcConfigValues final {
  explicit RpcConfigValues(const dynamic_config::Snapshot& config);

  bool enforce_task_deadline;
};

using ugrpc::impl::AsyncMethodInvocation;

class RpcData final {
 public:
  explicit RpcData(CallParams&&);

  RpcData(RpcData&&) noexcept = delete;
  RpcData& operator=(RpcData&&) noexcept = delete;
  ~RpcData();

  const grpc::ClientContext& GetContext() const noexcept;

  grpc::ClientContext& GetContext() noexcept;

  std::string_view GetCallName() const noexcept;

  std::string_view GetClientName() const noexcept;

  tracing::Span& GetSpan() noexcept;

  grpc::CompletionQueue& GetQueue() const noexcept;

  const RpcConfigValues& GetConfigValues() const noexcept;

  const Middlewares& GetMiddlewares() const noexcept;

  void ResetSpan() noexcept;

  ugrpc::impl::RpcStatisticsScope& GetStatsScope() noexcept;

  void SetWritesFinished() noexcept;

  bool AreWritesFinished() const noexcept;

  void SetFinished() noexcept;

  bool IsFinished() const noexcept;

  bool IsDeadlinePropagated() const noexcept;

  void SetDeadlinePropagated() noexcept;

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
  std::string client_name_;
  std::string_view call_name_;
  bool writes_finished_{false};
  bool is_finished_{false};
  bool is_deadline_propagated_{false};

  std::optional<tracing::InPlaceSpan> span_;
  ugrpc::impl::RpcStatisticsScope stats_scope_;
  grpc::CompletionQueue& queue_;
  RpcConfigValues config_values_;
  const Middlewares& mws_;

  std::optional<AsyncMethodInvocation> invocation_;
  grpc::Status status_;
};

class FutureImpl final {
 public:
  explicit FutureImpl(RpcData& data) noexcept;

  virtual ~FutureImpl() noexcept = default;

  FutureImpl(FutureImpl&&) noexcept;
  FutureImpl& operator=(FutureImpl&&) noexcept;

  /// @brief Checks if the asynchronous call has completed
  ///        Note, that once user gets result, IsReady should not be called
  /// @return true if result ready
  [[nodiscard]] bool IsReady() const noexcept;

  RpcData* GetData() noexcept;
  void ClearData() noexcept;

 private:
  RpcData* data_;
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
                         bool throw_on_error);

template <typename GrpcStream>
void Finish(GrpcStream& stream, RpcData& data, bool throw_on_error) {
  PrepareFinish(data);
  grpc::Status status;
  AsyncMethodInvocation finish;
  stream.Finish(&status, finish.GetTag());
  ProcessFinishResult(data, finish.Wait(), status, throw_on_error);
}

void PrepareRead(RpcData& data);

template <typename GrpcStream, typename Response>
[[nodiscard]] bool Read(GrpcStream& stream, Response& response, RpcData& data) {
  PrepareRead(data);
  AsyncMethodInvocation read;
  stream.Read(&response, read.GetTag());
  return read.Wait();
}

template <typename GrpcStream, typename Response>
void ReadAsync(GrpcStream& stream, Response& response, RpcData& data) noexcept {
  PrepareRead(data);
  data.EmplaceAsyncMethodInvocation();
  auto& read = data.GetAsyncMethodInvocation();
  stream.Read(&response, read.GetTag());
}

void PrepareWrite(RpcData& data);

template <typename GrpcStream, typename Request>
bool Write(GrpcStream& stream, const Request& request,
           grpc::WriteOptions options, RpcData& data) {
  PrepareWrite(data);
  AsyncMethodInvocation write;
  stream.Write(request, options, write.GetTag());
  auto result = write.Wait();
  if (!result) {
    data.SetWritesFinished();
  }
  return result;
}

void PrepareWriteAndCheck(RpcData& data);

template <typename GrpcStream, typename Request>
void WriteAndCheck(GrpcStream& stream, const Request& request,
                   grpc::WriteOptions options, RpcData& data) {
  PrepareWriteAndCheck(data);
  AsyncMethodInvocation write;
  stream.Write(request, options, write.GetTag());
  CheckOk(data, write.Wait(), "WriteAndCheck");
}

template <typename GrpcStream>
bool WritesDone(GrpcStream& stream, RpcData& data) {
  PrepareWrite(data);
  data.SetWritesFinished();
  AsyncMethodInvocation writes_done;
  stream.WritesDone(writes_done.GetTag());
  return writes_done.Wait();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
