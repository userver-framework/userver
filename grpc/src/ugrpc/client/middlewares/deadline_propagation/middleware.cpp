#include "middleware.hpp"

#include <ugrpc/impl/internal_tag.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::deadline_propagation {

namespace {

template <class Duration>
void AddTimeoutMsToSpan(tracing::Span& span, Duration d) {
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d);
  span.AddTag(tracing::kTimeoutMs, ms.count());
}

void UpdateDeadline(impl::RpcData& data) {
  // Disable by config
  if (!data.GetConfigValues().enforce_task_deadline) {
    return;
  }

  auto& context = data.GetContext();

  const auto context_time_left =
      ugrpc::impl::ExtractDeadlineDuration(context.raw_deadline());
  const engine::Deadline task_deadline =
      server::request::GetTaskInheritedDeadline();

  const auto client_deadline_reachable =
      (context_time_left != engine::Deadline::Duration::max());
  if (!task_deadline.IsReachable() && !client_deadline_reachable) {
    // both unreachable
    return;
  }

  auto& span = data.GetSpan();
  if (!task_deadline.IsReachable() && client_deadline_reachable) {
    AddTimeoutMsToSpan(span, context_time_left);
    return;
  }

  UASSERT(task_deadline.IsReachable());
  const auto task_time_left = task_deadline.TimeLeft();

  if (!client_deadline_reachable || task_time_left < context_time_left) {
    span.AddTag("deadline_updated", true);
    data.SetDeadlinePropagated();

    context.set_deadline(ugrpc::impl::ToGprTimePoint(task_time_left));

    AddTimeoutMsToSpan(span, task_time_left);
  } else {
    AddTimeoutMsToSpan(span, context_time_left);
  }
}

}  // namespace

void Middleware::Handle(MiddlewareCallContext& context) const {
  UpdateDeadline(context.GetCall().GetData(ugrpc::impl::InternalTag{}));

  context.Next();
}

std::shared_ptr<const MiddlewareBase> MiddlewareFactory::GetMiddleware(
    std::string_view /*client_name*/) const {
  return std::make_shared<Middleware>();
}

}  // namespace ugrpc::client::middlewares::deadline_propagation

USERVER_NAMESPACE_END
