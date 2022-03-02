#pragma once

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/server_context.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>
#include <userver/utils/lazy_prvalue.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/static_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/impl/async_service.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>
#include <userver/ugrpc/server/rpc.hpp>
#include <userver/ugrpc/server/service_base.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ReportHandlerError(const std::exception& ex,
                        std::string_view call_name) noexcept;

void ReportNetworkError(const RpcInterruptedError& ex,
                        std::string_view call_name) noexcept;

/// Per-gRPC-service data
template <typename GrpcppService>
struct ServiceData final {
  ServiceData(const ServiceSettings& settings,
              const ugrpc::impl::StaticServiceMetadata& metadata)
      : settings(settings), metadata(metadata) {
    statistics_holder =
        statistics.Register("server", settings.statistics_storage);
  }

  const ServiceSettings settings;
  const ugrpc::impl::StaticServiceMetadata metadata;
  AsyncService<GrpcppService> async_service{metadata.method_count};
  utils::impl::WaitTokenStorage wait_tokens;
  ugrpc::impl::ServiceStatistics statistics{metadata};
  utils::statistics::Entry statistics_holder;
};

/// Per-gRPC-method data
template <typename GrpcppService, typename CallTraits>
struct MethodData final {
  ServiceData<GrpcppService>& service_data;
  const std::size_t method_id{};
  typename CallTraits::ServiceBase& service;
  const typename CallTraits::ServiceMethod service_method;

  std::string_view call_name{
      service_data.metadata.method_full_names[method_id]};
  ugrpc::impl::MethodStatistics& statistics{
      service_data.statistics.GetMethodStatistics(method_id)};
};

template <typename GrpcppService, typename CallTraits>
class CallData final {
 public:
  explicit CallData(const MethodData<GrpcppService, CallTraits>& method_data)
      : wait_token_(method_data.service_data.wait_tokens.GetToken()),
        method_data_(method_data) {
    UASSERT(method_data.method_id <
            method_data.service_data.metadata.method_count);

    // the request for an incoming RPC must be performed synchronously
    auto& queue = method_data_.service_data.settings.queue;
    method_data_.service_data.async_service.template Prepare<CallTraits>(
        method_data_.method_id, context_, initial_request_, raw_responder_,
        queue, queue, prepare_.GetTag());
  }

  void operator()() && {
    if (!prepare_.Wait()) {
      // the CompletionQueue is shutting down
      return;
    }

    // start a concurrent listener immediately, as advised by gRPC docs
    ListenAsync(method_data_);

    HandleRpc();
  }

  static void ListenAsync(const MethodData<GrpcppService, CallTraits>& data) {
    // The new CallData is constructed synchronously and starts keeping
    // 'reactor' alive using its 'wait_token_'.
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
    const auto call_name = method_data_.call_name;
    auto& service = method_data_.service;
    const auto service_method = method_data_.service_method;

    tracing::Span span(std::string{call_name});
    ugrpc::impl::RpcStatisticsScope statistics_scope(method_data_.statistics);
    Call responder(context_, call_name, raw_responder_, statistics_scope);

    try {
      if constexpr (std::is_same_v<InitialRequest, NoInitialRequest>) {
        (service.*service_method)(responder);
      } else {
        (service.*service_method)(responder, std::move(initial_request_));
      }
    } catch (const RpcInterruptedError& ex) {
      ReportNetworkError(ex, call_name);
      statistics_scope.OnNetworkError();
    } catch (const std::exception& ex) {
      ReportHandlerError(ex, call_name);
    }
  }

  // 'wait_token_' must be the first field, because its lifetime keeps
  // ServiceData alive during server shutdown.
  const utils::impl::WaitTokenStorage::Token wait_token_;

  MethodData<GrpcppService, CallTraits> method_data_;

  grpc::ServerContext context_{};
  InitialRequest initial_request_{};
  RawCall raw_responder_{&context_};
  AsyncMethodInvocation prepare_{};
};

template <typename GrpcppService>
class ServiceWorkerImpl final : public ServiceWorker {
 public:
  template <typename Service, typename... ServiceMethods>
  ServiceWorkerImpl(ServiceSettings&& settings,
                    ugrpc::impl::StaticServiceMetadata&& metadata,
                    Service& service, ServiceMethods... service_methods)
      : service_data_(settings, metadata) {
    start_ = [this, &service, service_methods...] {
      std::size_t method_id = 0;
      (CallData<GrpcppService, CallTraits<ServiceMethods>>::ListenAsync(
           {service_data_, method_id++, service, service_methods}),
       ...);
    };
  }

  ~ServiceWorkerImpl() override {
    service_data_.wait_tokens.WaitForAllTokens();
  }

  grpc::Service& GetService() override { return service_data_.async_service; }

  const ugrpc::impl::StaticServiceMetadata& GetMetadata() const override {
    return service_data_.metadata;
  }

  void Start() override { start_(); }

 private:
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
      ServiceSettings(settings),
      ugrpc::impl::MakeStaticServiceMetadata<GrpcppService>(method_full_names),
      service, service_methods...);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
