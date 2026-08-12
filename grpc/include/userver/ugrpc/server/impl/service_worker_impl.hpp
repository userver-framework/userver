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
#include <userver/engine/impl/detach.hpp>
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

constexpr grpc::StatusCode kRatelimitedStatusCode{grpc::StatusCode::RESOURCE_EXHAUSTED};

struct GenericMethodParseResults {
    std::string_view call_name;
    std::string_view service_name;
    std::string_view method_name;
};
GenericMethodParseResults ParseGenericMethodName(std::string_view generic_method_name);

template <typename CallTraits, typename AsyncService>
void RequestAsyncCall(
    AsyncService& async_service,
    int method_id,
    typename CallTraits::RawContext& server_context,
    typename CallTraits::SerializedInitialRequest& serialized_initial_request,
    typename CallTraits::RawResponder& stream,
    grpc::ServerCompletionQueue& cq,
    void* tag
) {
    if constexpr (!std::is_same_v<ugrpc::impl::AsyncGenericService, AsyncService>) {
        if constexpr (IsSingleRequestMethod(CallTraits::kRpcType)) {
            impl::RequestAsyncCall(
                async_service,
                method_id,
                &server_context,
                &serialized_initial_request,
                &stream,
                &cq,
                &cq,
                tag
            );
        } else {
            impl::RequestAsyncCall(async_service, method_id, &server_context, &stream, &cq, &cq, tag);
        }
    } else {
        impl::RequestAsyncCall(async_service, &server_context, &stream, &cq, &cq, tag);
    }
}

template <typename CallTraits>
void FinishWithError(typename CallTraits::RawResponder& responder, const grpc::Status& status, void* tag) {
    if constexpr (IsSingleResponseMethod(CallTraits::kRpcType)) {
        responder.FinishWithError(status, tag);
    } else {
        responder.Finish(status, tag);
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

template <typename GrpcppService, typename CallTraits>
class AsyncCallProcessor {
public:
    using ServerContext = typename CallTraits::RawContext;
    using SerializedInitialRequest = typename CallTraits::SerializedInitialRequest;
    using Responder = typename CallTraits::RawResponder;

    AsyncCallProcessor(
        MethodData<GrpcppService, CallTraits>& method_data,
        ServerContext& server_context,
        SerializedInitialRequest& request,
        Responder& responder
    )
        : method_data_{method_data},
          server_context_{server_context},
          request_{request},
          responder_{responder}
    {}

    template <typename Call>
    void ProcessAsync(Call& call) {
        intrusive_ptr_add_ref(&call);

        utils::FastScopeGuard overloaded_guard([this, &call]() noexcept {
            ProcessRatelimited(call);
            intrusive_ptr_release(&call);
        });

        auto process_call_task = engine::AsyncNoTracing(
            method_data_.service_data.internals.task_processor,
            [this, &call, overloaded_guard = std::move(overloaded_guard)]() mutable {
                overloaded_guard.Release();

                SetCancellationToken(engine::current_task::GetCancellationToken());

                ProcessCall();

                intrusive_ptr_release(&call);
            }
        );

        engine::impl::DetachUnscopedUnsafeNoCancellationOnShutdown(
            utils::impl::InternalTag{},
            std::move(process_call_task)
        );
    }

    void RequestCancel() {
        const auto token_registered = cancellation_flag_.exchange(true, std::memory_order_acq_rel);
        if (token_registered) {
            // cancel task if token is registered
            cancellation_token_.RequestCancel();
        }
    }

private:
    void ProcessCall() {
        const auto& metadata = method_data_.service_data.metadata;
        std::string_view call_name = GetMethodFullName(metadata, method_data_.method_id);
        std::string_view service_name = metadata.service_full_name;
        std::string_view method_name = GetMethodName(metadata, method_data_.method_id);
        if constexpr (std::is_same_v<typename CallTraits::Context, GenericCallContext>) {
            auto parse_results = ParseGenericMethodName(server_context_.method());
            call_name = parse_results.call_name;
            service_name = parse_results.service_name;
            method_name = parse_results.method_name;
        }

        CallProcessor<CallTraits> call_processor{
            CallParams{
                server_context_,
                CallTraits::kRpcType,
                call_name,
                service_name,
                method_name,
                method_data_.method_statistics,
                method_data_.service_data.internals.statistics_storage,
                method_data_.service_data.internals.middlewares,
                method_data_.service_data.internals.config_source,
                method_data_.service_data.internals.status_codes_log_level,
                method_data_.service_data.internals.otel_trace_sampling_enabled,
            },
            responder_,
            request_,
            method_data_.service,
            method_data_.service_method,
        };

        call_processor.ProcessCall();
    }

    template <typename Call>
    void ProcessRatelimited(Call& call) {
        impl::AddRatelimitMetadata(server_context_);
        call.FinishWithError(grpc::Status{kRatelimitedStatusCode, "Congestion control: rate limit exceeded"});

        ugrpc::impl::RpcStatisticsScope stats_scope{method_data_.method_statistics};
        stats_scope.OnExplicitFinish(kRatelimitedStatusCode);
    }

    void SetCancellationToken(engine::TaskCancellationToken cancellation_token) {
        cancellation_token_ = std::move(cancellation_token);

        const auto rpc_cancelled = cancellation_flag_.exchange(true, std::memory_order_acq_rel);
        if (rpc_cancelled) {
            // `OnDoneEvent::Notify` already happened and RPC is cancelled, so cancel task manually
            cancellation_token_.RequestCancel();
        }
    }

    MethodData<GrpcppService, CallTraits>& method_data_;

    ServerContext& server_context_;
    SerializedInitialRequest& request_;
    Responder& responder_;

    engine::TaskCancellationToken cancellation_token_;

    // True if CancellationToken registered
    // or RPC IsCancelled.
    std::atomic<bool> cancellation_flag_{false};
};

template <typename GrpcppService, typename CallTraits>
class CallData final : public boost::intrusive_ref_counter<CallData<GrpcppService, CallTraits>> {
public:
    static void AcceptCall(const MethodData<GrpcppService, CallTraits>& method_data) {
        auto calld = utils::make_intrusive_ptr<CallData>(method_data);

        // Based on the tensorflow code, we must first call AsyncNotifyWhenDone
        // and only then RequestCall<>
        // see
        // https://git.ecdf.ed.ac.uk/s1886313/tensorflow/-/blob/438604fc885208ee05f9eef2d0f2c630e1360a83/tensorflow/core/distributed_runtime/rpc/grpc_call.h#L201
        // and grpc::ServerContext::AsyncNotifyWhenDone
        calld->server_context_.AsyncNotifyWhenDone(calld->on_done_.GetCompletionTag());

        calld->RequestAsyncCall();
    }

    explicit CallData(const MethodData<GrpcppService, CallTraits>& method_data)
        : wait_token_(method_data.service_data.wait_tokens.GetToken()),
          method_data_(method_data)
    {}

    void OnAccept(bool ok) {
        if (!ok) {
            // the CompletionQueue is shutting down

            // Do not wait for notify_when_done. When queue is shutting down, it will
            // not be called.
            // https://github.com/grpc/grpc/issues/10136
            on_done_.Release();
            return;
        }

        // request for another call immediately, as advised by gRPC docs
        CallData::AcceptCall(method_data_);

        UASSERT(!engine::current_task::IsTaskProcessorThread());
        async_call_processor_.ProcessAsync(*this);
    }

    void OnDone() {
        if (server_context_.IsCancelled()) {
            async_call_processor_.RequestCancel();
        }
    }

    void FinishWithError(grpc::Status status) {
        impl::FinishWithError<CallTraits>(responder_, status, on_finish_.GetCompletionTag());
    }

private:
    using ServerContext = typename CallTraits::RawContext;
    using SerializedInitialRequest = typename CallTraits::SerializedInitialRequest;
    using Responder = typename CallTraits::RawResponder;

    class Event final : public ugrpc::impl::EventBase {
    public:
        enum class EventType { kAccept, kFinish, kDone };

        Event(CallData& calld, EventType event_type)
            : calld_{calld},
              event_type_{event_type}
        {}

        void* GetCompletionTag() noexcept {
            // Ref for CompletionQueue; released in `Notify`.
            intrusive_ptr_add_ref(&calld_);

            return static_cast<EventBase*>(this);
        }

        void Notify(bool ok) noexcept override {
            switch (event_type_) {
                case EventType::kAccept:
                    calld_.OnAccept(ok);
                    break;
                case EventType::kFinish:
                    // No special handling needed apart from Release below.
                    break;
                case EventType::kDone:
                    // Server-side AsyncNotifyWhenDone: ok should always be true
                    UASSERT(ok);
                    calld_.OnDone();
                    break;
            }

            Release();
        }

        void Release() noexcept { intrusive_ptr_release(&calld_); }

    private:
        CallData& calld_;
        EventType event_type_{};
    };

    void RequestAsyncCall() {
        auto& cq = method_data_.service_data.internals.completion_queues.GetQueue(method_data_.queue_id);

        impl::RequestAsyncCall<CallTraits>(
            method_data_.service_data.async_service,
            method_data_.method_id,
            server_context_,
            serialized_initial_request_,
            responder_,
            cq,
            on_accept_.GetCompletionTag()
        );
    }

    // 'wait_token_' must be the first field, because its lifetime keeps
    // ServiceData alive during server shutdown.
    const utils::impl::WaitTokenStorageLock wait_token_;

    MethodData<GrpcppService, CallTraits> method_data_;

    ServerContext server_context_;
    SerializedInitialRequest serialized_initial_request_{};
    Responder responder_{&server_context_};

    AsyncCallProcessor<GrpcppService, CallTraits>
        async_call_processor_{method_data_, server_context_, serialized_initial_request_, responder_};

    Event on_accept_{*this, Event::EventType::kAccept};
    Event on_finish_{*this, Event::EventType::kFinish};
    Event on_done_{*this, Event::EventType::kDone};
};

template <typename GrpcppService, typename ServiceMethod>
void StartProcessing(const MethodData<GrpcppService, CallTraits<ServiceMethod>>& method_data) {
    CallData<GrpcppService, CallTraits<ServiceMethod>>::AcceptCall(method_data);
}

template <typename GrpcppService, typename Service, typename... ServiceMethods>
void StartProcessing(ServiceData<GrpcppService>& service_data, Service& service, ServiceMethods... service_methods) {
    for (std::size_t queue_id = 0; queue_id < service_data.internals.completion_queues.GetSize(); ++queue_id) {
        std::size_t method_id = 0;
        (impl::StartProcessing<
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
              impl::StartProcessing(service_data_, service, service_methods...);
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
