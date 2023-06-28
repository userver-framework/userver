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

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
