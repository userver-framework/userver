#pragma once

#include <string_view>

#include <grpcpp/server_context.h>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/tracing/in_place_span.hpp>

#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

grpc::Status ReportHandlerError(
    const std::exception& ex,
    std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept;

grpc::Status ReportNetworkError(
    const RpcInterruptedError& ex,
    std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept;

grpc::Status ReportCustomError(
    const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex,
    std::string_view call_name,
    tracing::Span& span
);

template <typename ResultType>
grpc::Status ExtractErrorStatus(ResultType& result) {
    UASSERT(!result.IsSuccess());
    if constexpr (meta::kIsInstantiationOf<Result, ResultType>) {
        return result.ExtractErrorStatus();
    } else if (meta::kIsInstantiationOf<StreamingResult, ResultType>) {
        return result.ExtractStatus();
    } else {
        UINVARIANT(false, "Unknow result type");
    }
}

template <typename ResultType>
auto ExtractResponse(ResultType&& result) {
    if constexpr (meta::kIsInstantiationOf<Result, ResultType>) {
        return std::forward<ResultType>(result).ExtractResponse();
    } else if (meta::kIsInstantiationOf<StreamingResult, ResultType>) {
        return std::forward<ResultType>(result).ExtractLastResponse();
    } else {
        UINVARIANT(false, "Unknow result type");
    }
}

template <typename CallTraits, typename DoHandleFunc>
class CallProcessor final {
public:
    using Call = typename CallTraits::Call;
    using Response = typename CallTraits::Response;

    CallProcessor(
        MiddlewareCallContext& context,
        Call& call,
        const Middlewares& mids,
        ugrpc::impl::RpcStatisticsScope& statistics_scope,
        ::google::protobuf::Message* initial_request,
        DoHandleFunc&& do_handle
    )
        : context_(context),
          call_(call),
          mids_(mids),
          statistics_scope_(statistics_scope),
          initial_request_(initial_request),
          do_handle_(std::move(do_handle)) {}

    void DoCall() {
        context_.SetStatusPtr(&status_);
        RunOnCallStart();

        if (!status_.ok()) {
            FinishOnError();
            return;
        }

        // The snapshot won't valid in OnCallFinish.
        context_.ResetInitialDynamicConfig(utils::impl::InternalTag{});
        std::optional<decltype(do_handle_())> result_opt{};
        RunWithCatch([this, &result_opt] { result_opt.emplace(do_handle_()); });

        // streaming handler can detect rpc braekage during a network interaction
        if (call_.IsFinished()) {
            return;
        }

        if (!status_.ok()) {
            FinishOnError();
            return;
        }

        UASSERT(result_opt.has_value());

        auto& result = result_opt.value();

        if (!result.IsSuccess()) {
            status_ = impl::ExtractErrorStatus(result);
            FinishOnError();
            return;
        }

        RunWithCatch([this, &result] {
            if constexpr (meta::kIsInstantiationOf<StreamingResult, std::remove_reference_t<decltype(result)>>) {
                if (!result.HasLastResponse()) {
                    RunOnCallFinish();
                    call_.Finish();
                    return;
                }
            }
            auto response = impl::ExtractResponse(std::move(result));
            RunPreSendMessage(response);
            RunOnCallFinish();
            if (!status_.ok()) {
                call_.FinishWithError(status_);
            } else {
                call_.Finish(response);
            }
        });
    }

private:
    void RunOnCallStart() {
        UASSERT(success_pre_hooks_count_ == 0);
        for (const auto& m : mids_) {
            RunWithCatch([this, &m] { m->OnCallStart(context_); });
            if (!status_.ok()) {
                return;
            }
            // On fail, we must call OnRpcFinish only for middlewares for which OnRpcStart has been called successfully.
            // So, we watch to count of these middlewares.
            ++success_pre_hooks_count_;
            if (initial_request_) {
                RunWithCatch([this, m] { m->PostRecvMessage(context_, *initial_request_); });
                if (!status_.ok()) {
                    return;
                }
            }
        }
    }

    void RunOnCallFinish() {
        const auto rbegin = mids_.rbegin() + (mids_.size() - success_pre_hooks_count_);
        for (auto it = rbegin; it != mids_.rend(); ++it) {
            const auto& middleware = *it;
            // We must call all OnRpcFinish despite the failures. So, don't check the status.
            RunWithCatch([this, &middleware] { middleware->OnCallFinish(context_, status_); });
        }
    }

    void RunPreSendMessage(Response& response) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, Response>) {
            // We don't want to include a heavy boost header for reverse view.
            // NOLINTNEXTLINE(modernize-loop-convert)
            for (auto it = mids_.rbegin(); it != mids_.rend(); ++it) {
                const auto& middleware = *it;
                RunWithCatch([this, &response, &middleware] { middleware->PreSendMessage(context_, response); });
                if (!status_.ok()) {
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
            status_ = ex.ExtractStatus();
        } catch (const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex) {
            status_ = ReportCustomError(ex, context_.GetCallName(), context_.GetSpan());
        } catch (const RpcInterruptedError& ex) {
            status_ = ReportNetworkError(ex, context_.GetCallName(), context_.GetSpan(), statistics_scope_);
        } catch (const std::exception& ex) {
            status_ = ReportHandlerError(ex, context_.GetCallName(), context_.GetSpan(), statistics_scope_);
        }
    }

    void FinishOnError() {
        UASSERT(!call_.IsFinished());
        UASSERT(!status_.ok());
        RunOnCallFinish();
        try {
            call_.FinishWithError(status_);
        } catch (const RpcInterruptedError& ex) {
            [[maybe_unused]] const auto st =
                ReportNetworkError(ex, context_.GetCallName(), context_.GetSpan(), statistics_scope_);
        }
    }

private:
    MiddlewareCallContext& context_;
    Call& call_;
    const Middlewares& mids_;
    ugrpc::impl::RpcStatisticsScope& statistics_scope_;
    grpc::Status status_;
    ::google::protobuf::Message* initial_request_{nullptr};
    std::size_t success_pre_hooks_count_{0};
    DoHandleFunc do_handle_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
