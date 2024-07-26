#pragma once

#include <chrono>
#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/server_context.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <userver/ugrpc/impl/static_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/impl/async_method_invocation.hpp>
#include <userver/ugrpc/server/impl/async_service.hpp>
#include <userver/ugrpc/server/impl/call_params.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/error_code.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/rpc.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ReportHandlerError(
    const std::exception& ex, std::string_view call_name, tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope) noexcept;

void ReportNetworkError(
    const RpcInterruptedError& ex, std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope) noexcept;

void ReportCustomError(
    const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex,
    CallAnyBase& call, tracing::Span& span);

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder,
               grpc::ServerContext& context, std::string_view call_name);

void ParseGenericCallName(std::string_view generic_call_name,
                          std::string_view& call_name,
                          std::string_view& service_name,
                          std::string_view& method_name);

/// Per-gRPC-service data
template <typename GrpcppService>
struct ServiceData final {
  ServiceData(const ServiceSettings& settings,
              const ugrpc::impl::StaticServiceMetadata& metadata,
              ugrpc::impl::ServiceStatistics* service_statistics)
      : settings(settings),
        metadata(metadata),
        service_statistics(service_statistics) {}

  ~ServiceData() { wait_tokens.WaitForAllTokens(); }

  const ServiceSettings settings;
  const ugrpc::impl::StaticServiceMetadata metadata;
  AsyncService<GrpcppService> async_service{metadata.method_full_names.size()};
  utils::impl::WaitTokenStorage wait_tokens;
  ugrpc::impl::ServiceStatistics* const service_statistics;
};

/// Per-gRPC-method data
template <typename GrpcppService, typename CallTraits>
struct MethodData final {
  ServiceData<GrpcppService>& service_data;
  const std::size_t queue_id{};
  const std::size_t method_id{};
  typename CallTraits::ServiceBase& service;
  const typename CallTraits::ServiceMethod service_method;

  std::string_view call_name{
      service_data.metadata.method_full_names[method_id]};
  // Remove name of the service and slash
  std::string_view method_name{
      call_name.substr(service_data.metadata.service_full_name.size() + 1)};
  ugrpc::impl::MethodStatistics* const statistics{
      service_data.service_statistics
          ? &service_data.service_statistics->GetMethodStatistics(method_id)
          : nullptr};
};

template <typename GrpcppService, typename CallTraits>
class CallData final {
 public:
  explicit CallData(const MethodData<GrpcppService, CallTraits>& method_data)
      : wait_token_(method_data.service_data.wait_tokens.GetToken()),
        method_data_(method_data) {
    UASSERT(method_data.method_id <
            method_data.service_data.metadata.method_full_names.size());
  }

  void operator()() && {
    // Based on the tensorflow code, we must first call AsyncNotifyWhenDone
    // and only then Prepare<>
    // see
    // https://git.ecdf.ed.ac.uk/s1886313/tensorflow/-/blob/438604fc885208ee05f9eef2d0f2c630e1360a83/tensorflow/core/distributed_runtime/rpc/grpc_call.h#L201
    // and grpc::ServerContext::AsyncNotifyWhenDone
    ugrpc::server::impl::RpcFinishedEvent notify_when_done(
        engine::current_task::GetCancellationToken(), context_);

    context_.AsyncNotifyWhenDone(notify_when_done.GetTag());

    auto& queue = method_data_.service_data.settings.queue.GetQueue(
        method_data_.queue_id);

    // the request for an incoming RPC must be performed synchronously
    method_data_.service_data.async_service.template Prepare<CallTraits>(
        method_data_.method_id, context_, initial_request_, raw_responder_,
        queue, queue, prepare_.GetTag());

    // Note: we ignore task cancellations here. Even if notify_when_done has
    // already cancelled this RPC, we want to:
    // 1. listen to further RPCs for the same method
    // 2. handle this RPC correctly, including metrics, logs, etc.
    if (Wait(prepare_) != impl::AsyncMethodInvocation::WaitStatus::kOk) {
      // the CompletionQueue is shutting down

      // Do not wait for notify_when_done. When queue is shutting down, it will
      // not be called.
      // https://github.com/grpc/grpc/issues/10136
      return;
    }

    utils::FastScopeGuard await_notify_when_done([&]() noexcept {
      // Even if we finished before receiving notification that call is done, we
      // should wait on this async operation. CompletionQueue has a pointer to
      // stack-allocated object, that object is going to be freed upon exit. To
      // prevent segfaults, wait until queue is done with this object.
      notify_when_done.Wait();
    });

    // start a concurrent listener immediately, as advised by gRPC docs
    ListenAsync(method_data_);

    HandleRpc();
  }

  static void ListenAsync(const MethodData<GrpcppService, CallTraits>& data) {
    engine::CriticalAsyncNoSpan(
        data.service_data.settings.task_processor,
        utils::LazyPrvalue([&] { return CallData(data); }))
        .Detach();
  }

 private:
  using InitialRequest = typename CallTraits::InitialRequest;
  using RawCall = typename CallTraits::RawCall;
  using Call = typename CallTraits::Call;

  void HandleRpc() {
    auto call_name = method_data_.call_name;
    auto service_name = method_data_.service_data.metadata.service_full_name;
    auto method_name = method_data_.method_name;
    if constexpr (CallTraits::kCallCategory == CallCategory::kGeneric) {
      ParseGenericCallName(context_.method(), call_name, service_name,
                           method_name);
    }

    const auto& middlewares = method_data_.service_data.settings.middlewares;

    SetupSpan(span_, context_, call_name);
    utils::FastScopeGuard destroy_span([&]() noexcept { span_.reset(); });

    ugrpc::impl::RpcStatisticsScope statistics_scope{
        method_data_.statistics
            ? *method_data_.statistics
            : method_data_.service_data.settings.statistics_storage
                  .GetGenericStatistics(call_name, std::nullopt)};

    auto& access_tskv_logger =
        method_data_.service_data.settings.access_tskv_logger;
    utils::AnyStorage<StorageContext> storage_context;
    Call responder(CallParams{context_, call_name, service_name, method_name,
                              statistics_scope, *access_tskv_logger,
                              span_->Get(), storage_context, middlewares},
                   raw_responder_);
    auto do_call = [&] {
      if constexpr (std::is_same_v<InitialRequest, NoInitialRequest>) {
        (method_data_.service.*(method_data_.service_method))(responder);
      } else {
        (method_data_.service.*(method_data_.service_method))(
            responder, std::move(initial_request_));
      }
    };

    try {
      ::google::protobuf::Message* initial_request = nullptr;
      if constexpr (!std::is_same_v<InitialRequest, NoInitialRequest>) {
        initial_request = &initial_request_;
      }

      MiddlewareCallContext middleware_context(
          middlewares, responder, do_call,
          method_data_.service_data.settings.config_source.GetSnapshot(),
          initial_request);
      responder.RunMiddlewarePipeline(middleware_context);
    } catch (
        const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex) {
      ReportCustomError(ex, responder, span_->Get());
    } catch (const RpcInterruptedError& ex) {
      ReportNetworkError(ex, call_name, span_->Get(), statistics_scope);
    } catch (const std::exception& ex) {
      ReportHandlerError(ex, call_name, span_->Get(), statistics_scope);
    }
  }

  // 'wait_token_' must be the first field, because its lifetime keeps
  // ServiceData alive during server shutdown.
  const utils::impl::WaitTokenStorage::Token wait_token_;

  MethodData<GrpcppService, CallTraits> method_data_;

  typename CallTraits::ContextType context_{};
  InitialRequest initial_request_{};
  RawCall raw_responder_{&context_};
  ugrpc::impl::AsyncMethodInvocation prepare_;
  std::optional<tracing::InPlaceSpan> span_{};
};

template <typename GrpcppService, typename Service, typename... ServiceMethods>
void StartServing(ServiceData<GrpcppService>& service_data, Service& service,
                  ServiceMethods... service_methods) {
  for (std::size_t queue_id = 0;
       queue_id < service_data.settings.queue.GetSize(); ++queue_id) {
    std::size_t method_id = 0;
    (CallData<GrpcppService, CallTraits<ServiceMethods>>::ListenAsync(
         {service_data, queue_id, method_id++, service, service_methods}),
     ...);
  }
}

template <typename GrpcppService>
class ServiceWorkerImpl final : public ServiceWorker {
 public:
  template <typename Service, typename... ServiceMethods>
  ServiceWorkerImpl(ServiceSettings&& settings,
                    ugrpc::impl::StaticServiceMetadata&& metadata,
                    Service& service, ServiceMethods... service_methods)
      : statistics_(settings.statistics_storage.GetServiceStatistics(
            metadata, std::nullopt)),
        service_data_(std::move(settings), std::move(metadata), &statistics_),
        start_([this, &service, service_methods...] {
          impl::StartServing(service_data_, service, service_methods...);
        }) {}

  grpc::Service& GetService() override { return service_data_.async_service; }

  const ugrpc::impl::StaticServiceMetadata& GetMetadata() const override {
    return service_data_.metadata;
  }

  void Start() override { start_(); }

 private:
  ugrpc::impl::ServiceStatistics& statistics_;
  ServiceData<GrpcppService> service_data_;
  std::function<void()> start_;
};

// Called from 'MakeWorker' of code-generated service base classes
template <typename GrpcppService, typename Service, typename... ServiceMethods>
std::unique_ptr<ServiceWorker> MakeServiceWorker(
    ServiceSettings&& settings,
    const std::string_view (&method_full_names)[sizeof...(ServiceMethods)],
    Service& service, ServiceMethods... service_methods) {
  return std::make_unique<ServiceWorkerImpl<GrpcppService>>(
      std::move(settings),
      ugrpc::impl::MakeStaticServiceMetadata<GrpcppService>(method_full_names),
      service, service_methods...);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
