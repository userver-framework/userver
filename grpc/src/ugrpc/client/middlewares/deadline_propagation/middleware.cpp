#include <userver/ugrpc/client/middlewares/deadline_propagation/middleware.hpp>

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::deadline_propagation {

namespace {

template <class Duration>
void AddTimeoutMsToSpan(tracing::Span& span, Duration d) {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d);
    span.AddTag(tracing::kTimeoutMs, ms.count());
}

void UpdateDeadline(impl::CallState& state) {
    // Disable by config
    if (!state.GetConfigValues().enforce_task_deadline) {
        return;
    }

    auto& context = state.GetClientContext();

    const auto context_time_left = ugrpc::TimespecToDuration(context.raw_deadline());
    const engine::Deadline task_deadline = USERVER_NAMESPACE::server::request::GetTaskInheritedDeadline();

    const auto client_deadline_reachable = (context_time_left != engine::Deadline::Duration::max());
    if (!task_deadline.IsReachable() && !client_deadline_reachable) {
        // both unreachable
        return;
    }

    auto& span = state.GetSpan();
    if (!task_deadline.IsReachable() && client_deadline_reachable) {
        AddTimeoutMsToSpan(span, context_time_left);
        return;
    }

    UASSERT(task_deadline.IsReachable());
    const auto task_time_left = task_deadline.TimeLeft();

    if (!client_deadline_reachable || task_time_left < context_time_left) {
        span.AddTag("deadline_updated", true);
        state.SetDeadlinePropagated();

        context.set_deadline(ugrpc::DurationToTimespec(task_time_left));

        AddTimeoutMsToSpan(span, task_time_left);
    } else {
        AddTimeoutMsToSpan(span, context_time_left);
    }
}

}  // namespace

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    UpdateDeadline(context.GetState(utils::impl::InternalTag{}));
}

}  // namespace ugrpc::client::middlewares::deadline_propagation

USERVER_NAMESPACE_END
