#pragma once

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/trx_tracker.hpp>

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
          cancellation_token_{perform_task_} {}

    ~AsyncUnaryCallAdapter() override { unary_call_.Abandon(); }

    AsyncUnaryCallAdapter(AsyncUnaryCallAdapter&&) = delete;
    AsyncUnaryCallAdapter& operator=(AsyncUnaryCallAdapter&&) = delete;

    CallContext& GetContext() noexcept override { return unary_call_.GetContext(); }
    const CallContext& GetContext() const noexcept override { return unary_call_.GetContext(); }

    bool IsReady() const override { return perform_task_.IsFinished(); }

    engine::FutureStatus WaitUntil(engine::Deadline deadline) const noexcept override {
        utils::trx_tracker::CheckNoTransactions(unary_call_.GetContext().GetCallName());
        return perform_task_.WaitNothrowUntil(deadline);
    }

    Response Get() override {
        const auto status = WaitUntil(engine::Deadline{});
        if (status != engine::FutureStatus::kReady) {
            perform_task_.RequestCancel();
        }
        const engine::TaskCancellationBlocker cancel_blocker;
        return perform_task_.Get();
    }

    void Cancel() override { cancellation_token_.RequestCancel(); }

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
        return perform_task_.TryGetContextAccessor();
    }

private:
    Request request_;
    UnaryCall<Stub, Request, Response> unary_call_;
    engine::TaskWithResult<Response> perform_task_;
    engine::TaskCancellationToken cancellation_token_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
