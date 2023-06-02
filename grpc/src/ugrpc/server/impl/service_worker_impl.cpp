#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

#include <chrono>

#include <grpc/support/time.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/impl/rpc_metadata_keys.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <ugrpc/server/impl/server_configs.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

std::optional<engine::Deadline> TryExtractDeadline(
    std::chrono::system_clock::time_point time) {
  auto duration = time - std::chrono::system_clock::now();
  if (duration >= std::chrono::hours{365 * 24}) {
    return std::nullopt;
  }
  return engine::Deadline::FromDuration(duration);
}

}  // namespace

void ReportHandlerError(const std::exception& ex, std::string_view call_name,
                        tracing::Span& span) noexcept {
  LOG_ERROR() << "Uncaught exception in '" << call_name << "': " << ex;
  span.AddTag(tracing::kErrorFlag, true);
  span.AddTag(tracing::kErrorMessage, ex.what());
}

void ReportNetworkError(const RpcInterruptedError& ex,
                        std::string_view call_name,
                        tracing::Span& span) noexcept {
  LOG_WARNING() << "Network error in '" << call_name << "': " << ex;
  span.AddTag(tracing::kErrorFlag, true);
  span.AddTag(tracing::kErrorMessage, ex.what());
}

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder,
               grpc::ServerContext& context, std::string_view call_name) {
  auto span_name = utils::StrCat("grpc/", call_name);
  const auto& client_metadata = context.client_metadata();

  const auto* const trace_id =
      utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaTraceId);
  const auto* const parent_span_id =
      utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaSpanId);
  if (trace_id && parent_span_id) {
    span_holder.emplace(std::move(span_name), ugrpc::impl::ToString(*trace_id),
                        ugrpc::impl::ToString(*parent_span_id),
                        utils::impl::SourceLocation::Current());
  } else {
    span_holder.emplace(std::move(span_name),
                        utils::impl::SourceLocation::Current());
  }

  auto& span = span_holder->Get();

  const auto* const parent_link =
      utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaRequestId);
  if (parent_link) {
    span.SetParentLink(ugrpc::impl::ToString(*parent_link));
  }

  context.AddInitialMetadata(ugrpc::impl::kXYaTraceId,
                             ugrpc::impl::ToGrpcString(span.GetTraceId()));
  context.AddInitialMetadata(ugrpc::impl::kXYaSpanId,
                             ugrpc::impl::ToGrpcString(span.GetSpanId()));
  context.AddInitialMetadata(ugrpc::impl::kXYaRequestId,
                             ugrpc::impl::ToGrpcString(span.GetLink()));
}

bool CheckAndSetupDeadline(tracing::Span& span, grpc::ServerContext& context,
                           const std::string& service_name,
                           const std::string& method_name,
                           ugrpc::impl::RpcStatisticsScope& statistics_scope,
                           dynamic_config::Snapshot config) {
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
      config[kServerCancelTaskByDeadline]) {
    // Experiment and config are enabled
    statistics_scope.CancelledByDeadlinePropagation();
    return false;
  }

  USERVER_NAMESPACE::server::request::TaskInheritedData inherited_data{
      &service_name, method_name, std::chrono::steady_clock::now(), deadline};
  USERVER_NAMESPACE::server::request::kTaskInheritedData.Set(inherited_data);

  return true;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
