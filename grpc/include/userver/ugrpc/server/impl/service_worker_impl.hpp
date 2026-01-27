#pragma once

#include <exception>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
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

struct GenericMethodParseResults {
    std::string_view call_name;
    std::string_view service_name;
    std::string_view method_name;
};
GenericMethodParseResults ParseGenericMethodName(std::string_view generic_method_name);

void ConstructSpan(
    std::optional<tracing::InPlaceSpan>& span_storage,
    std::string_view call_name,
    const grpc::ServerContext& server_context
);

void AddServiceMethodTags(tracing::Span& span, std::string_view service_name, std::string_view method_name);

/// Per-gRPC-service data
template <typename GrpcppService>
struct ServiceData final {
    ServiceData(ServiceInternals&& internals, const ugrpc::impl::StaticServiceMetadata& metadata)
        : internals(std::move(internals)),
          metadata(metadata)
    {}

    ~ServiceData() { wait_tokens.WaitForAllTokens(); }

    const ServiceInternals internals;
    const ugrpc::impl::StaticServiceMetadata metadata;
    AsyncService<GrpcppService> async_service{GetMethodsCount(metadata)};
    utils::impl::WaitTokenStorage wait_tokens;
    ugrpc::impl::ServiceStatistics& service_statistics{
        internals.statistics_storage.GetServiceStatistics(metadata, std::nullopt)
    };
};

/// Per-gRPC-method data
template <typename GrpcppService, typename CallTraits>
struct MethodData final {
    ServiceData<GrpcppService>& service_data;
    const std::size_t queue_id{};
    const std::size_t method_id{};
    typename CallTraits::ServiceBase& service;
    const typename CallTraits::ServiceMethod service_method;

    utils::StringLiteral call_name{ugrpc::impl::GetMethodFullName(service_data.metadata, method_id)};
    // Remove name of the service and slash
    utils::StringLiteral method_name{ugrpc::impl::GetMethodName(service_data.metadata, method_id)};
    ugrpc::impl::MethodStatistics& statistics{service_data.service_statistics.GetMethodStatistics(method_id)};
};

template <typename GrpcppService, typename CallTraits>
class CallData final {
public:
    explicit CallData(const MethodData<GrpcppService, CallTraits>& method_data)
        : wait_token_(method_data.service_data.wait_tokens.GetToken()),
          method_data_(method_data)
    {
        UASSERT(method_data.method_id < GetMethodsCount(method_data.service_data.metadata));
    }

    void operator()() && {
        // Based on the tensorflow code, we must first call AsyncNotifyWhenDone
        // and only then RequestCall<>
        // see
        // https://git.ecdf.ed.ac.uk/s1886313/tensorflow/-/blob/438604fc885208ee05f9eef2d0f2c630e1360a83/tensorflow/core/distributed_runtime/rpc/grpc_call.h#L201
        // and grpc::ServerContext::AsyncNotifyWhenDone
        ugrpc::server::impl::RpcDoneEvent
            notify_when_done{engine::current_task::GetCancellationToken(), server_context_};
        server_context_.AsyncNotifyWhenDone(notify_when_done.GetCompletionTag());

        auto& queue = method_data_.service_data.internals.completion_queues.GetQueue(method_data_.queue_id);

        ugrpc::impl::AsyncMethodInvocation request_call_invocation;
        // the request for an incoming RPC must be performed synchronously
        method_data_.service_data.async_service.template RequestCall<CallTraits>(
            method_data_.method_id,
            server_context_,
            initial_request_,
            raw_responder_,
            queue,
            queue,
            request_call_invocation.GetCompletionTag()
        );

        // Note: we ignore task cancellations here. Even if notify_when_done has
        // already cancelled this RPC, we want to:
        // 1. listen to further RPCs for the same method
        // 2. handle this RPC correctly, including metrics, logs, etc.
        if (!request_call_invocation.WaitNonCancellable()) {
            // the CompletionQueue is shutting down

            // Do not wait for notify_when_done. When queue is shutting down, it will
            // not be called.
            // https://github.com/grpc/grpc/issues/10136
            return;
        }

        const utils::FastScopeGuard await_notify_when_done([&]() noexcept {
            // Even if we finished before receiving notification that call is done, we
            // should wait on this async operation. CompletionQueue has a pointer to
            // stack-allocated object, that object is going to be freed upon exit. To
            // prevent segfaults, wait until queue is done with this object.
            notify_when_done.WaitNonCancellable();
        });

        // start a concurrent listener immediately, as advised by gRPC docs
        ListenAsync(method_data_);

        ProcessCall();
    }

    static void ListenAsync(const MethodData<GrpcppService, CallTraits>& method_data) {
        engine::DetachUnscopedUnsafe(engine::CriticalAsyncNoSpan(
            method_data.service_data.internals.task_processor,
            utils::LazyPrvalue([&] { return CallData(method_data); })
        ));
    }

private:
    using InitialRequest = typename CallTraits::InitialRequest;
    using RawResponder = typename CallTraits::RawResponder;
    using Context = typename CallTraits::Context;

    void ProcessCall() {
        std::string_view call_name = method_data_.call_name;
        std::string_view service_name = method_data_.service_data.metadata.service_full_name;
        std::string_view method_name = method_data_.method_name;
        if constexpr (std::is_same_v<Context, GenericCallContext>) {
            auto parse_results = ParseGenericMethodName(server_context_.method());
            call_name = parse_results.call_name;
            service_name = parse_results.service_name;
            method_name = parse_results.method_name;
        }

        ConstructSpan(span_, call_name, server_context_);
        AddServiceMethodTags(span_->Get(), service_name, method_name);

        CallProcessor<CallTraits> call_processor{
            CallParams{
                server_context_,
                CallTraits::kRpcType,
                call_name,
                service_name,
                method_name,
                method_data_.statistics,
                method_data_.service_data.internals.statistics_storage,
                span_->Get(),
                method_data_.service_data.internals.middlewares,
                method_data_.service_data.internals.config_source,
                method_data_.service_data.internals.status_codes_log_level,
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
    const utils::impl::WaitTokenStorageLock wait_token_;

    MethodData<GrpcppService, CallTraits> method_data_;

    typename CallTraits::RawContext server_context_{};
    InitialRequest initial_request_{};
    RawResponder raw_responder_{&server_context_};
    std::optional<tracing::InPlaceSpan> span_;
};

template <typename GrpcppService, typename Service, typename... ServiceMethods>
void StartServing(ServiceData<GrpcppService>& service_data, Service& service, ServiceMethods... service_methods) {
    for (std::size_t queue_id = 0; queue_id < service_data.internals.completion_queues.GetSize(); ++queue_id) {
        std::size_t method_id = 0;
        (CallData<
             GrpcppService,
             CallTraits<ServiceMethods>>::ListenAsync({service_data, queue_id, method_id++, service, service_methods}),
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
        : service_data_(std::move(internals), std::move(metadata)),
          start_([this, &service, service_methods...] {
              impl::StartServing(service_data_, service, service_methods...);
          })
    {}

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
    const std::array<ugrpc::impl::MethodDescriptor, sizeof...(ServiceMethods)>& methods,
    Service& service,
    ServiceMethods... service_methods
) {
    return std::make_unique<ServiceWorkerImpl<GrpcppService>>(
        std::move(internals),
        ugrpc::impl::MakeStaticServiceMetadata<GrpcppService>(methods),
        service,
        service_methods...
    );
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
