#include <userver/tracing/manager.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

bool DefaultTracingManager::TryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request, SpanBuilder& span_builder) const {
  const auto& trace_id = request.GetHeader(http::headers::kXYaTraceId);
  if (!trace_id.empty()) span_builder.SetTraceId(std::move(trace_id));

  span_builder.SetParentSpanId(request.GetHeader(http::headers::kXYaSpanId));

  const auto& parent_link = request.GetHeader(http::headers::kXYaRequestId);
  if (!parent_link.empty()) span_builder.SetParentLink(parent_link);

  return true;
}

void DefaultTracingManager::FillRequestWithTracingContext(
    const tracing::Span& span,
    clients::http::RequestTracingEditor request) const {
  request.SetHeader(http::headers::kXYaRequestId, span.GetLink());
  request.SetHeader(http::headers::kXYaTraceId, span.GetTraceId());
  request.SetHeader(http::headers::kXYaSpanId, span.GetSpanId());
}

void DefaultTracingManager::FillResponseWithTracingContext(
    const Span& span, server::http::HttpResponse& response) const {
  response.SetHeader(http::headers::kXYaRequestId, span.GetLink());
  response.SetHeader(http::headers::kXYaTraceId, span.GetTraceId());
  response.SetHeader(http::headers::kXYaSpanId, span.GetSpanId());
}

const DefaultTracingManager kDefaultTracingManager{};

}  // namespace tracing

USERVER_NAMESPACE_END
