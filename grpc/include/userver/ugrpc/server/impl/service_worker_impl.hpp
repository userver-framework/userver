#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <grpcpp/server_context.h>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/async_service.hpp>
#include <userver/ugrpc/impl/event_base.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/call_context.hpp>
#include <userver/ugrpc/server/impl/call_processor.hpp>
#include <userver/ugrpc/server/impl/call_state.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/server/impl/error_code.hpp>
#include <userver/ugrpc/server/impl/ratelimit_metadata.hpp>
#include <userver/ugrpc/server/impl/request_async_call.hpp>
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

template <typename CallTraits, typename AsyncService>
bool RequestAsyncCall(
    AsyncService& async_service,
    int method_id,
    typename CallTraits::RawContext& server_context,
    typename CallTraits::InitialRequest& initial_request,
    typename CallTraits::RawResponder& stream,
    grpc::ServerCompletionQueue& cq
) {
    ugrpc::impl::AsyncMethodInvocation request_call_invocation;

    void* tag = request_call_invocation.GetCompletionTag();

    if constexpr (!std::is_same_v<ugrpc::impl::AsyncGenericService, AsyncService>) {
        if constexpr (IsSingleRequestMethod(CallTraits::kRpcType)) {
            RequestAsyncCall(async_service, method_id, &server_context, &initial_request, &stream, &cq, &cq, tag);
        } else {
            RequestAsyncCall(async_service, method_id, &server_context, &stream, &cq, &cq, tag);
        }
    } else {
        RequestAsyncCall(async_service, &server_context, &stream, &cq, &cq, tag);
    }

    return request_call_invocation.WaitNonCancellable();
}

template <typename CallTraits>
void FinishRatelimited(typename CallTraits::RawResponder& responder, grpc::StatusCode status_code) {
    grpc::Status status{status_code, "Congestion control: rate limit exceeded"};
    if constexpr (IsSingleResponseMethod(CallTraits::kRpcType)) {
        [[maybe_unused]] const bool ok = impl::FinishWithError(responder, status);
    } else {
        [[maybe_unused]] const bool ok = impl::Finish(responder, status);
    }
}

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
    ugrpc::impl::AsyncService<GrpcppService> async_service;
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
    ugrpc::impl::MethodStatistics& method_statistics{service_data.service_statistics.GetMethodStatistics(method_id)};
};

template <typename CallTraits>
struct CallData final : public boost::intrusive_ref_counter<CallData<CallTraits>> {
    class OnDoneEvent final : public ugrpc::impl::EventBase {
    public:
        explicit OnDoneEvent(CallData& calld)
            : calld_{calld}
        {}

        void* GetCompletionTag() noexcept {
            // Ref for CompletionQueue; released in `Notify`.
            intrusive_ptr_add_ref(&calld_);

            return static_cast<EventBase*>(this);
        }

        void Notify(bool ok) noexcept override {
            // Server-side AsyncNotifyWhenDone: ok should always be true
            UASSERT(ok);

            if (calld_.server_context.IsCancelled()) {
                const auto token_registered = calld_.cancellation_flag.exchange(true, std::memory_order_acq_rel);
                if (token_registered) {
                    // cancel task if token is registered
                    calld_.cancellation_token.RequestCancel();
                }
            }

            Release();
        }

        void Release() noexcept { intrusive_ptr_release(&calld_); }

    private:
        CallData& calld_;
    };

    using ServerContext = typename CallTraits::RawContext;
    using InitialRequest = typename CallTraits::InitialRequest;
    using Responder = typename CallTraits::RawResponder;

    ServerContext server_context;
    InitialRequest initial_request{};
    Responder responder{&server_context};

    engine::TaskCancellationToken cancellation_token{engine::current_task::GetCancellationToken()};

    // True if CancellationToken registered
    // or RPC IsCancelled.
    std::atomic<bool> cancellation_flag{false};

    OnDoneEvent on_done_event{*this};
};

template <typename GrpcppService, typename CallTraits>
class ProcessWrapper {
public:
    ProcessWrapper(
        const MethodData<GrpcppService, CallTraits>& method_data,
        boost::intrusive_ptr<CallData<CallTraits>>&& calld
    )
        : wait_token_(method_data.service_data.wait_tokens.GetToken()),
          method_data_(method_data),
          calld_{std::move(calld)}
    {}

    ~ProcessWrapper() {
        if (overloaded_) {
            ProcessOverloaded();
        }
    }

    void operator()() && {
        overloaded_ = false;

        calld_->cancellation_token = engine::current_task::GetCancellationToken();

        const auto rpc_cancelled = calld_->cancellation_flag.exchange(true, std::memory_order_acq_rel);
        if (rpc_cancelled) {
            // `OnDoneEvent::Notify` already happened and RPC is cancelled, so cancel task manually
            engine::current_task::RequestCancel();
        }

        ProcessCall();
    }

private:
    void ProcessCall() {
        const auto& metadata = method_data_.service_data.metadata;
        std::string_view call_name = GetMethodFullName(metadata, method_data_.method_id);
        std::string_view service_name = metadata.service_full_name;
        std::string_view method_name = GetMethodName(metadata, method_data_.method_id);
        if constexpr (std::is_same_v<typename CallTraits::Context, GenericCallContext>) {
            auto parse_results = ParseGenericMethodName(calld_->server_context.method());
            call_name = parse_results.call_name;
            service_name = parse_results.service_name;
            method_name = parse_results.method_name;
        }

        CallProcessor<CallTraits> call_processor{
            CallParams{
                calld_->server_context,
                CallTraits::kRpcType,
                call_name,
                service_name,
                method_name,
                method_data_.method_statistics,
                method_data_.service_data.internals.statistics_storage,
                method_data_.service_data.internals.middlewares,
                method_data_.service_data.internals.config_source,
                method_data_.service_data.internals.status_codes_log_level,
            },
            calld_->responder,
            calld_->initial_request,
            method_data_.service,
            method_data_.service_method,
        };

        call_processor.DoCall();
    }

    void ProcessOverloaded() {
        constexpr grpc::StatusCode kRatelimitedStatusCode{grpc::StatusCode::RESOURCE_EXHAUSTED};

        ugrpc::impl::RpcStatisticsScope stats_scope{method_data_.method_statistics};

        impl::AddRatelimitMetadata(calld_->server_context);

        impl::FinishRatelimited<CallTraits>(calld_->responder, kRatelimitedStatusCode);

        stats_scope.OnExplicitFinish(kRatelimitedStatusCode);

        // Do not construct span
    }

    const utils::impl::WaitTokenStorageLock wait_token_;

    MethodData<GrpcppService, CallTraits> method_data_;

    boost::intrusive_ptr<CallData<CallTraits>> calld_;

    bool overloaded_{true};
};

template <typename GrpcppService, typename CallTraits>
void ProcessAsync(
    const MethodData<GrpcppService, CallTraits>& method_data,
    boost::intrusive_ptr<CallData<CallTraits>>&& calld
) {
    engine::DetachUnscopedUnsafe(engine::AsyncNoTracing(
        method_data.service_data.internals.task_processor,
        utils::LazyPrvalue([&]() mutable { return ProcessWrapper{method_data, std::move(calld)}; })
    ));
}

template <typename GrpcppService, typename CallTraits>
class CallAcceptor {
public:
    explicit CallAcceptor(const MethodData<GrpcppService, CallTraits>& method_data)
        : wait_token_(method_data.service_data.wait_tokens.GetToken()),
          method_data_(method_data)
    {}

    void operator()() && {
        while (auto calld = AcceptCall()) {
            impl::ProcessAsync(method_data_, std::move(calld));
        }
    }

private:
    boost::intrusive_ptr<CallData<CallTraits>> AcceptCall() {
        auto calld = utils::make_intrusive_ptr<CallData<CallTraits>>();

        // Based on the tensorflow code, we must first call AsyncNotifyWhenDone
        // and only then RequestCall<>
        // see
        // https://git.ecdf.ed.ac.uk/s1886313/tensorflow/-/blob/438604fc885208ee05f9eef2d0f2c630e1360a83/tensorflow/core/distributed_runtime/rpc/grpc_call.h#L201
        // and grpc::ServerContext::AsyncNotifyWhenDone
        calld->server_context.AsyncNotifyWhenDone(calld->on_done_event.GetCompletionTag());

        auto& cq = method_data_.service_data.internals.completion_queues.GetQueue(method_data_.queue_id);

        // the request for an incoming RPC must be performed synchronously
        const auto ok = RequestAsyncCall<CallTraits>(
            method_data_.service_data.async_service,
            method_data_.method_id,
            calld->server_context,
            calld->initial_request,
            calld->responder,
            cq
        );

        // Note: we ignore task cancellations here. Even if notify_when_done has
        // already cancelled this RPC, we want to:
        // 1. listen to further RPCs for the same method
        // 2. handle this RPC correctly, including metrics, logs, etc.
        if (!ok) {
            // the CompletionQueue is shutting down

            // Do not wait for notify_when_done. When queue is shutting down, it will
            // not be called.
            // https://github.com/grpc/grpc/issues/10136
            calld->on_done_event.Release();

            return nullptr;
        }

        return calld;
    }

    // 'wait_token_' must be the first field, because its lifetime keeps
    // ServiceData alive during server shutdown.
    const utils::impl::WaitTokenStorageLock wait_token_;

    MethodData<GrpcppService, CallTraits> method_data_;
};

template <typename GrpcppService, typename ServiceMethod>
void ListenAsync(const MethodData<GrpcppService, CallTraits<ServiceMethod>>& method_data) {
    engine::DetachUnscopedUnsafe(engine::CriticalAsyncNoTracing(
        method_data.service_data.internals.task_processor,
        utils::LazyPrvalue([&] { return CallAcceptor{method_data}; })
    ));
}

template <typename GrpcppService, typename Service, typename... ServiceMethods>
void StartServing(ServiceData<GrpcppService>& service_data, Service& service, ServiceMethods... service_methods) {
    for (std::size_t queue_id = 0; queue_id < service_data.internals.completion_queues.GetSize(); ++queue_id) {
        std::size_t method_id = 0;
        (impl::ListenAsync<
             GrpcppService,
             ServiceMethods>({service_data, queue_id, method_id++, service, service_methods}),
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
