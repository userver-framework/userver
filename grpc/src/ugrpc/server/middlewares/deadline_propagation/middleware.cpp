#include "middleware.hpp"

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/server/handlers/impl/deadline_propagation_config.hpp>
#include <userver/server/request/task_inherited_data.hpp>

#include <ugrpc/impl/internal_tag.hpp>
#include <ugrpc/server/impl/server_configs.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

namespace {

bool CheckAndSetupDeadline(tracing::Span& span, grpc::ServerContext& context,
                           std::string_view service_name,
                           std::string_view method_name,
                           ugrpc::impl::RpcStatisticsScope& statistics_scope,
                           const dynamic_config::Snapshot& config) {
  if (!config[USERVER_NAMESPACE::server::handlers::impl::
                  kDeadlinePropagationEnabled]) {
    return true;
  }

  auto deadline_duration =
      ugrpc::impl::ExtractDeadlineDuration(context.raw_deadline());
  if (deadline_duration == engine::Deadline::Duration::max()) {
    return true;
  }

  auto deadline_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(deadline_duration);

  const bool cancel_by_deadline =
      context.IsCancelled() || deadline_ms <= engine::Deadline::Duration{0};

  span.AddNonInheritableTag("received_deadline_ms", deadline_ms.count());
  statistics_scope.OnDeadlinePropagated();
  span.AddNonInheritableTag("cancelled_by_deadline", cancel_by_deadline);

  if (cancel_by_deadline && config[impl::kServerCancelTaskByDeadline]) {
    // Experiment and config are enabled
    statistics_scope.OnCancelledByDeadlinePropagation();
    return false;
  }

  auto deadline = engine::Deadline::FromDuration(deadline_duration);
  USERVER_NAMESPACE::server::request::TaskInheritedData inherited_data{
      service_name, method_name, std::chrono::steady_clock::now(), deadline};
  USERVER_NAMESPACE::server::request::kTaskInheritedData.Set(inherited_data);

  return true;
}

}  // namespace

void Middleware::Handle(MiddlewareCallContext& context) const {
  auto& call = context.GetCall();

  if (!CheckAndSetupDeadline(call.GetSpan(), call.GetContext(),
                             context.GetServiceName(), context.GetMethodName(),
                             call.Statistics(ugrpc::impl::InternalTag()),
                             context.GetInitialDynamicConfig())) {
    call.FinishWithError(grpc::Status{
        grpc::StatusCode::DEADLINE_EXCEEDED,
        "Deadline propagation: Not enough time to handle this call"});
    return;
  }

  context.Next();
}

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
