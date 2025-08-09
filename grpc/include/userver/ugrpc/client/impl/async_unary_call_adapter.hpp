#pragma once

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/call_context.hpp>
#include <userver/ugrpc/client/impl/response_future_impl_base.hpp>
#include <userver/ugrpc/client/impl/unary_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Stub, typename Request, typename Response>
class AsyncUnaryCallAdapter final : public ResponseFutureImplBase<Response> {
public:
    AsyncUnaryCallAdapter(
        CallParams&& params,
        PrepareUnaryCallProxy<Stub, Request, Response>&& prepare_unary_call,
        const Request& request
    )
        : request_{request},
          unary_call_{std::move(params), std::move(prepare_unary_call), request_},
          perform_task_{utils::CriticalAsync(
              "async-unary-call-perform",
              [this] {
                  // TODO: get rid of span for perform_task
                  // we use `utils::CriticalAsync` to inherit TaskInheritedVariable, but it create span,
                  // so set LogLevel=None
                  tracing::Span::CurrentSpan().SetLogLevel(logging::Level::kNone);

                  unary_call_.Perform();
                  return unary_call_.ExtractResponse();
              }
          )},
          context_{
              utils::impl::InternalTag{},
              unary_call_.GetContext().GetState(utils::impl::InternalTag{}),
              [token = engine::TaskCancellationToken{perform_task_}]() mutable { token.RequestCancel(); }} {}

    ~AsyncUnaryCallAdapter() override { unary_call_.Abandon(); }

    AsyncUnaryCallAdapter(AsyncUnaryCallAdapter&&) = delete;
    AsyncUnaryCallAdapter& operator=(AsyncUnaryCallAdapter&&) = delete;

    CancellableCallContext& GetContext() noexcept override { return context_; }
    const CancellableCallContext& GetContext() const noexcept override { return context_; }

    bool IsReady() const override { return perform_task_.IsFinished(); }

    engine::FutureStatus WaitUntil(engine::Deadline deadline) const noexcept override {
        return perform_task_.WaitNothrowUntil(deadline);
    }

    Response Get() override {
        if (!perform_task_.WaitNothrow()) {
            perform_task_.RequestCancel();
        }
        const engine::TaskCancellationBlocker cancel_blocker;
        return perform_task_.Get();
    }

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
        return perform_task_.TryGetContextAccessor();
    }

private:
    Request request_;
    UnaryCall<Stub, Request, Response> unary_call_;
    engine::TaskWithResult<Response> perform_task_;
    CancellableCallContext context_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
