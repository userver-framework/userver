#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <google/protobuf/message.h>
#include <grpcpp/server_context.h>

#include <userver/logging/log.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/impl/call_state.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/ugrpc/server/impl/rpc.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/result.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void SetupSpan(
    std::optional<tracing::InPlaceSpan>& span_storage,
    grpc::ServerContext& context,
    std::string_view call_name,
    std::string_view service_name,
    std::string_view method_name
);

grpc::Status ReportCustomError(const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex, CallState& state)
    noexcept;

grpc::Status ReportHandlerError(const std::exception& ex, CallState& state) noexcept;

void ReportFinished(const grpc::Status& status, CallState& state) noexcept;

void ReportInterrupted(CallState& state) noexcept;

template <typename Response>
void UnpackResult(Result<Response>&& result, std::optional<Response>& response, grpc::Status& status) {
    if (result.IsSuccess()) {
        response.emplace(result.ExtractResponse());
    } else {
        status = result.ExtractErrorStatus();
    }
}

template <typename Response>
void UnpackResult(StreamingResult<Response>&& result, std::optional<Response>& response, grpc::Status& status) {
    if (result.IsSuccess()) {
        if (result.HasLastResponse()) {
            response.emplace(result.ExtractLastResponse());
        }
    } else {
        status = result.ExtractStatus();
    }
}

template <typename CallTraits>
bool Finish(
    impl::Responder<CallTraits>& responder,
    const std::optional<typename CallTraits::Response>& response,
    grpc::Status& status
) {
    if (status.ok()) {
        if constexpr (!IsSingleResponseMethod(CallTraits::kRpcType)) {
            if (response.has_value()) {
                return responder.Finish(*response);
            } else {
                return responder.Finish();
            }
        } else {
            UINVARIANT(response.has_value(), "response should not be empty");
            return responder.Finish(*response);
        }
    } else {
        return responder.FinishWithError(status);
    }
}

template <typename CallTraits>
class CallProcessor final {
public:
    using Responder = impl::Responder<CallTraits>;
    using Response = typename CallTraits::Response;
    using InitialRequest = typename CallTraits::InitialRequest;
    using Context = typename CallTraits::Context;
    using ServiceBase = typename CallTraits::ServiceBase;
    using ServiceMethod = typename CallTraits::ServiceMethod;
    using RawResponder = typename CallTraits::RawResponder;

    CallProcessor(
        CallParams&& params,
        RawResponder& raw_responder,
        InitialRequest& initial_request,
        ServiceBase& service,
        ServiceMethod service_method
    )
        : state_(std::move(params)),
          responder_(state_, raw_responder),
          middleware_call_context_(utils::impl::InternalTag{}, state_, status_),
          initial_request_(initial_request),
          service_(service),
          service_method_(service_method)
    {}

    void DoCall() {
        auto scope_time = state_.GetSpan().CreateScopeTime("finish");

        RunOnCallStart();

        bool finished = false;
        const utils::FastScopeGuard post_finish_hooks_guard([this, &finished]() noexcept {
            RunOnCallFinish(finished ? std::make_optional(std::move(status_)) : std::nullopt);
        });

        // Don't keep the config snapshot for too long, especially for streaming RPCs.
        state_.config_snapshot.reset();

        std::optional<Response> response;
        if (!engine::current_task::ShouldCancel() && status_.ok()) {
            RunWithCatch([this, &response] {
                auto result = CallHandler();
                impl::UnpackResult(std::move(result), response, status_);
            });
        }

        if (!engine::current_task::ShouldCancel() && !responder_.IsInterrupted()) {
            RunPreFinishHooks(response);
            finished = impl::Finish(responder_, response, status_);
            scope_time.Reset("post_finish");
        }

        if (finished) {
            impl::ReportFinished(status_, state_);
        } else {
            impl::ReportInterrupted(state_);
        }
    }

private:
    auto CallHandler() {
        Context context{utils::impl::InternalTag{}, state_};

        if constexpr (!IsSingleRequestMethod(CallTraits::kRpcType)) {
            return (service_.*service_method_)(context, responder_);
        } else if constexpr (CallTraits::kRpcType == RpcType::kUnary) {
            return (service_.*service_method_)(context, std::move(initial_request_));
        } else if constexpr (CallTraits::kRpcType == RpcType::kServerStreaming) {
            return (service_.*service_method_)(context, std::move(initial_request_), responder_);
        } else {
            static_assert(!sizeof(CallTraits), "Unexpected RpcType");
        }
    }

    void RunOnCallStart() {
        UASSERT(success_pre_hooks_count_ == 0);
        for (const auto& m : state_.middlewares) {
            RunWithCatch([this, &m] { m->OnCallStart(middleware_call_context_); });
            if (!status_.ok()) {
                return;
            }
            // On fail, we must call OnRpcFinish only for middlewares for which OnRpcStart has been called successfully.
            // So, we watch to count of these middlewares.
            ++success_pre_hooks_count_;
            if constexpr (std::is_base_of_v<google::protobuf::Message, InitialRequest>) {
                RunWithCatch([this, &m] { m->PostRecvMessage(middleware_call_context_, initial_request_); });
                if (!status_.ok()) {
                    return;
                }
            }
        }
    }

    void RunPreFinishHooks(std::optional<Response>& response) {
        const auto& mids = state_.middlewares;
        const auto rbegin = mids.rbegin() + (mids.size() - success_pre_hooks_count_);
        for (auto it = rbegin; it != mids.rend(); ++it) {
            const auto& middleware = *it;

            if constexpr (std::is_base_of_v<google::protobuf::Message, Response>) {
                if (status_.ok() && response.has_value()) {
                    RunWithCatch([this, &middleware, &response] {
                        middleware->PreSendMessage(middleware_call_context_, *response);
                    });
                }
            }

            RunWithCatch([this, &middleware] { middleware->PreSendStatus(middleware_call_context_, status_); });
        }
    }

    void RunOnCallFinish(const std::optional<grpc::Status>& status) {
        const auto& mids = state_.middlewares;
        const auto rbegin = mids.rbegin() + (mids.size() - success_pre_hooks_count_);
        for (auto it = rbegin; it != mids.rend(); ++it) {
            const auto& middleware = *it;
            try {
                middleware->OnCallFinish(middleware_call_context_, status);
            } catch (const std::exception& ex) {
                LOG_WARNING() << "Error in OnCallFinish: " << ex;
            }
        }
    }

    template <typename Func>
    void RunWithCatch(Func&& func) {
        try {
            func();
        } catch (MiddlewareRpcInterruptionError& ex) {
            status_ = ex.ExtractStatus();
        } catch (ErrorWithStatus& ex) {
            status_ = ex.ExtractStatus();
        } catch (const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex) {
            status_ = impl::ReportCustomError(ex, state_);
        } catch (const RpcInterruptedError& /*ex*/) {
            UASSERT(responder_.IsInterrupted());
            // RPC interruption will be reported below.
        } catch (const std::exception& ex) {
            status_ = impl::ReportHandlerError(ex, state_);
        }
    }

    CallState state_;
    Responder responder_;
    grpc::Status status_;
    MiddlewareCallContext middleware_call_context_;
    // Initial request is the request which is sent to the service together with RPC initiation.
    // Unary-request RPCs have an initial request, client-streaming RPCs don't.
    InitialRequest& initial_request_;
    ServiceBase& service_;
    const ServiceMethod service_method_;

    std::size_t success_pre_hooks_count_{0};
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
