#pragma once

#include <exception>
#include <functional>
#include <optional>
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
#include <userver/tracing/span_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/call_context.hpp>
#include <userver/ugrpc/server/impl/async_method_invocation.hpp>
#include <userver/ugrpc/server/impl/async_service.hpp>
#include <userver/ugrpc/server/impl/call_processor.hpp>
#include <userver/ugrpc/server/impl/call_state.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/server/impl/error_code.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/ugrpc/server/impl/service_internals.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ParseGenericCallName(
    std::string_view generic_call_name,
    std::string_view& call_name,
    std::string_view& service_name,
    std::string_view& method_name
);

/// Per-gRPC-service data
template <typename GrpcppService>
struct ServiceData final {
    ServiceData(const ServiceInternals& internals, const ugrpc::impl::StaticServiceMetadata& metadata)
        : internals(internals), metadata(metadata) {}

    ~ServiceData() { wait_tokens.WaitForAllTokens(); }

    const ServiceInternals internals;
    const ugrpc::impl::StaticServiceMetadata metadata;
    AsyncService<GrpcppService> async_service{GetMethodsCount(metadata)};
    utils::impl::WaitTokenStorage wait_tokens;
    ugrpc::impl::ServiceStatistics& service_statistics{
        internals.statistics_storage.GetServiceStatistics(metadata, std::nullopt)};
};

/// Per-gRPC-method data
template <typename GrpcppService, typename CallTraits>
struct MethodData final {
    ServiceData<GrpcppService>& service_data;
    const std::size_t queue_id{};
    const std::size_t method_id{};
    typename CallTraits::ServiceBase& service;
    const typename CallTraits::ServiceMethod service_method;

    std::string_view call_name{GetMethodFullName(service_data.metadata, method_id)};
    // Remove name of the service and slash
    std::string_view method_name{GetMethodName(service_data.metadata, method_id)};
    ugrpc::impl::MethodStatistics& statistics{service_data.service_statistics.GetMethodStatistics(method_id)};
};

template <typename GrpcppService, typename CallTraits>
class CallData final {
public:
    explicit CallData(const MethodData<GrpcppService, CallTraits>& method_data)
        : wait_token_(method_data.service_data.wait_tokens.GetToken()), method_data_(method_data) {
        UASSERT(method_data.method_id < GetMethodsCount(method_data.service_data.metadata));
    }

    void operator()() && {
        // Based on the tensorflow code, we must first call AsyncNotifyWhenDone
        // and only then Prepare<>
        // see
        // https://git.ecdf.ed.ac.uk/s1886313/tensorflow/-/blob/438604fc885208ee05f9eef2d0f2c630e1360a83/tensorflow/core/distributed_runtime/rpc/grpc_call.h#L201
        // and grpc::ServerContext::AsyncNotifyWhenDone
        ugrpc::server::impl::RpcFinishedEvent notify_when_done(engine::current_task::GetCancellationToken(), context_);

        context_.AsyncNotifyWhenDone(notify_when_done.GetTag());

        auto& queue = method_data_.service_data.internals.completion_queues.GetQueue(method_data_.queue_id);

        // the request for an incoming RPC must be performed synchronously
        method_data_.service_data.async_service.template Prepare<CallTraits>(
            method_data_.method_id, context_, initial_request_, raw_responder_, queue, queue, prepare_.GetTag()
        );

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
            data.service_data.internals.task_processor, utils::LazyPrvalue([&] { return CallData(data); })
        ).Detach();
    }

private:
    using InitialRequest = typename CallTraits::InitialRequest;
    using RawResponder = typename CallTraits::RawResponder;
    using Context = typename CallTraits::Context;

    void HandleRpc() {
        auto call_name = method_data_.call_name;
        auto service_name = method_data_.service_data.metadata.service_full_name;
        auto method_name = method_data_.method_name;
        if constexpr (std::is_same_v<Context, GenericCallContext>) {
            ParseGenericCallName(context_.method(), call_name, service_name, method_name);
        }

        CallProcessor<CallTraits> call_processor{
            CallParams{
                context_,
                call_name,
                service_name,
                method_name,
                method_data_.statistics,
                method_data_.service_data.internals.statistics_storage,
                span_storage_,
                method_data_.service_data.internals.middlewares,
                method_data_.service_data.internals.config_source,
                *method_data_.service_data.internals.access_tskv_logger,
            },
            raw_responder_,
            initial_request_,
            method_data_.service,
            method_data_.service_method,
        };

        call_processor.DoCall();
    }

    // 'wait_token_' must be the first field, because its lifetime keeps
    // ServiceData alive during server shutdown.
    const utils::impl::WaitTokenStorage::Token wait_token_;

    MethodData<GrpcppService, CallTraits> method_data_;

    typename CallTraits::RawContext context_{};
    InitialRequest initial_request_{};
    RawResponder raw_responder_{&context_};
    ugrpc::impl::AsyncMethodInvocation prepare_;
    std::optional<tracing::InPlaceSpan> span_storage_{};
};

template <typename GrpcppService, typename Service, typename... ServiceMethods>
void StartServing(ServiceData<GrpcppService>& service_data, Service& service, ServiceMethods... service_methods) {
    for (std::size_t queue_id = 0; queue_id < service_data.internals.completion_queues.GetSize(); ++queue_id) {
        std::size_t method_id = 0;
        (CallData<GrpcppService, CallTraits<ServiceMethods>>::ListenAsync(
             {service_data, queue_id, method_id++, service, service_methods}
         ),
         ...);
    }
}

template <typename GrpcppService>
class ServiceWorkerImpl final : public ServiceWorker {
public:
    template <typename Service, typename... ServiceMethods>
    ServiceWorkerImpl(
        ServiceInternals&& internals,
        ugrpc::impl::StaticServiceMetadata&& metadata,
        Service& service,
        ServiceMethods... service_methods
    )
        : service_data_(std::move(internals), std::move(metadata)), start_([this, &service, service_methods...] {
              impl::StartServing(service_data_, service, service_methods...);
          }) {}

    grpc::Service& GetService() override { return service_data_.async_service; }

    const ugrpc::impl::StaticServiceMetadata& GetMetadata() const override { return service_data_.metadata; }

    void Start() override { start_(); }

private:
    ServiceData<GrpcppService> service_data_;
    std::function<void()> start_;
};

// Called from 'MakeWorker' of code-generated service base classes
template <typename GrpcppService, typename Service, typename... ServiceMethods>
std::unique_ptr<ServiceWorker> MakeServiceWorker(
    ServiceInternals&& internals,
    const std::string_view (&method_full_names)[sizeof...(ServiceMethods)],
    Service& service,
    ServiceMethods... service_methods
) {
    return std::make_unique<ServiceWorkerImpl<GrpcppService>>(
        std::move(internals),
        ugrpc::impl::MakeStaticServiceMetadata<GrpcppService>(method_full_names),
        service,
        service_methods...
    );
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
