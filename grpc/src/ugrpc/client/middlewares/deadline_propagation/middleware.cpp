#include "middleware.hpp"

#include <ugrpc/impl/internal_tag.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::deadline_propagation {

namespace {

void UpdateDeadline(impl::RpcData& data) {
  // Disable by config
  if (!data.GetConfigValues().enforce_task_deadline) {
    return;
  }

  auto& span = data.GetSpan();
  auto& context = data.GetContext();

  const auto context_deadline =
      engine::Deadline::FromTimePoint(context.deadline());
  const engine::Deadline task_deadline =
      server::request::GetTaskInheritedDeadline();

  if (!task_deadline.IsReachable() && !context_deadline.IsReachable()) {
    return;
  }

  engine::Deadline result_deadline{context_deadline};

  if (task_deadline < context_deadline) {
    span.AddTag("deadline_updated", true);
    data.SetDeadlinePropagated();
    result_deadline = task_deadline;
  }

  auto result_deadline_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          result_deadline.TimeLeft());

  context.set_deadline(result_deadline);
  span.AddTag(tracing::kTimeoutMs, result_deadline_ms.count());
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
