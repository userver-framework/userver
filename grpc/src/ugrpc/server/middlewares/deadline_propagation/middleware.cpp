#include "middleware.hpp"

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/impl/internal_tag.hpp>
#include <ugrpc/server/impl/server_configs.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

namespace {

std::optional<engine::Deadline> TryExtractDeadline(
    std::chrono::system_clock::time_point time) {
  const auto duration = time - std::chrono::system_clock::now();
  if (duration >= std::chrono::hours{365 * 24}) {
    return std::nullopt;
  }
  return engine::Deadline::FromDuration(duration);
}

bool CheckAndSetupDeadline(tracing::Span& span, grpc::ServerContext& context,
                           std::string_view service_name,
                           std::string_view method_name,
                           ugrpc::impl::RpcStatisticsScope& statistics_scope,
                           const dynamic_config::Snapshot& config) {
  auto opt_deadline = TryExtractDeadline(context.deadline());
  if (!opt_deadline) {
    return true;
  }

  auto deadline = *opt_deadline;
  const bool cancel_by_deadline = context.IsCancelled() || deadline.IsReached();

  auto deadline_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline.TimeLeft());
  span.AddNonInheritableTag("received_deadline_ms", deadline_ms.count());
  statistics_scope.OnDeadlinePropagated();
  span.AddNonInheritableTag("cancelled_by_deadline", cancel_by_deadline);

  if (cancel_by_deadline &&
      utils::impl::kGrpcServerDeadlinePropagationExperiment.IsEnabled() &&
      config[impl::kServerCancelTaskByDeadline]) {
    // Experiment and config are enabled
    statistics_scope.CancelledByDeadlinePropagation();
    return false;
  }

  USERVER_NAMESPACE::server::request::TaskInheritedData inherited_data{
      service_name, method_name, std::chrono::steady_clock::now(), deadline};
  USERVER_NAMESPACE::server::request::kTaskInheritedData.Set(inherited_data);

  return true;
}

}  // namespace

void Middleware::Handle(MiddlewareCallContext& context) const {
  auto& call = context.GetCall();

  if (!CheckAndSetupDeadline(call.GetCallSpan(), call.GetContext(),
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
