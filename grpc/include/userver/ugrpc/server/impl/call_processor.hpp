#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <google/protobuf/message.h>
#include <grpcpp/server_context.h>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/impl/call_kind.hpp>
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
    std::string_view call_name
);

grpc::Status ReportHandlerError(const std::exception& ex, CallState& state) noexcept;

void ReportRpcInterruptedError(CallState& state) noexcept;

grpc::Status
ReportCustomError(const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex, CallState& state) noexcept;

void CheckFinishStatus(bool finish_op_succeeded, const grpc::Status& status, CallState& state) noexcept;

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
        : state_(std::move(params), CallTraits::kCallKind),
          responder_(state_, raw_responder),
          context_(utils::impl::InternalTag{}, state_),
          initial_request_(initial_request),
          service_(service),
          service_method_(service_method) {
        // TODO Move setting up Span a middleware?
        SetupSpan(state_.span_storage, state_.server_context, state_.call_name);
    }

    void DoCall() {
        RunOnCallStart();

        // Don't keep the config snapshot for too long, especially for streaming RPCs.
        state_.config_snapshot.reset();

        if (!Status().ok()) {
            RunOnCallFinish();
            impl::CheckFinishStatus(responder_.FinishWithError(Status()), Status(), state_);
            return;
        }

        // Final response is the response sent to the client together with status in the final batch.
        std::optional<Response> final_response{};

        RunWithCatch([this, &final_response] {
            auto result = CallHandler();
            impl::UnpackResult(std::move(result), final_response, Status());
        });

        // Streaming handler can detect RPC breakage during a network interaction => IsFinished.
        // RpcFinishedEvent can signal RPC interruption while in the handler => ShouldCancel.
        if (responder_.IsFinished() || engine::current_task::ShouldCancel()) {
            impl::ReportRpcInterruptedError(state_);
            // Don't run OnCallFinish.
            return;
        }

        if (!Status().ok()) {
            RunOnCallFinish();
            impl::CheckFinishStatus(responder_.FinishWithError(Status()), Status(), state_);
            return;
        }

        if (final_response) {
            RunPreSendMessage(*final_response);
        }
        RunOnCallFinish();

        if (!Status().ok()) {
            impl::CheckFinishStatus(responder_.FinishWithError(Status()), Status(), state_);
            return;
        }

        if constexpr (IsServerStreaming(CallTraits::kCallKind)) {
            if (!final_response) {
                impl::CheckFinishStatus(responder_.Finish(), Status(), state_);
                return;
            }
        }
        UASSERT(final_response);
        impl::CheckFinishStatus(responder_.Finish(*final_response), Status(), state_);
    }

private:
    auto CallHandler() {
        Context context{utils::impl::InternalTag{}, state_};

        if constexpr (impl::IsClientStreaming(CallTraits::kCallKind)) {
            return (service_.*service_method_)(context, responder_);
        } else if constexpr (CallTraits::kCallKind == CallKind::kUnaryCall) {
            return (service_.*service_method_)(context, std::move(initial_request_));
        } else if constexpr (CallTraits::kCallKind == CallKind::kOutputStream) {
            return (service_.*service_method_)(context, std::move(initial_request_), responder_);
        } else {
            static_assert(!sizeof(CallTraits), "Unexpected CallCategory");
        }
    }

    void RunOnCallStart() {
        UASSERT(success_pre_hooks_count_ == 0);
        for (const auto& m : state_.middlewares) {
            RunWithCatch([this, &m] { m->OnCallStart(context_); });
            if (!Status().ok()) {
                return;
            }
            // On fail, we must call OnRpcFinish only for middlewares for which OnRpcStart has been called successfully.
            // So, we watch to count of these middlewares.
            ++success_pre_hooks_count_;
            if constexpr (std::is_base_of_v<google::protobuf::Message, InitialRequest>) {
                RunWithCatch([this, &m] { m->PostRecvMessage(context_, initial_request_); });
                if (!Status().ok()) {
                    return;
                }
            }
        }
    }

    void RunOnCallFinish() {
        const auto& mids = state_.middlewares;
        const auto rbegin = mids.rbegin() + (mids.size() - success_pre_hooks_count_);
        for (auto it = rbegin; it != mids.rend(); ++it) {
            const auto& middleware = *it;
            // We must call all OnRpcFinish despite the failures. So, don't check the status.
            RunWithCatch([this, &middleware] { middleware->OnCallFinish(context_, Status()); });
        }
    }

    void RunPreSendMessage(Response& response) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, Response>) {
            const auto& mids = state_.middlewares;
            // We don't want to include a heavy boost header for reverse view.
            // NOLINTNEXTLINE(modernize-loop-convert)
            for (auto it = mids.rbegin(); it != mids.rend(); ++it) {
                const auto& middleware = *it;
                RunWithCatch([this, &response, &middleware] { middleware->PreSendMessage(context_, response); });
                if (!Status().ok()) {
                    return;
                }
            }
        }
    }

    template <typename Func>
    void RunWithCatch(Func&& func) {
        try {
            func();
        } catch (MiddlewareRpcInterruptionError& ex) {
            Status() = ex.ExtractStatus();
        } catch (ErrorWithStatus& ex) {
            Status() = ex.ExtractStatus();
        } catch (const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex) {
            Status() = impl::ReportCustomError(ex, state_);
        } catch (const RpcInterruptedError& /*ex*/) {
            UASSERT(responder_.IsFinished());
            // RPC interruption will be reported below.
        } catch (const std::exception& ex) {
            Status() = impl::ReportHandlerError(ex, state_);
        }
    }

    grpc::Status& Status() { return context_.GetStatus(utils::impl::InternalTag{}); }

    CallState state_;
    Responder responder_;
    MiddlewareCallContext context_;
    // Initial request is the request which is sent to the service together with RPC initiation.
    // Unary-request RPCs have an initial request, client-streaming RPCs don't.
    InitialRequest& initial_request_;
    ServiceBase& service_;
    const ServiceMethod service_method_;

    std::size_t success_pre_hooks_count_{0};
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
