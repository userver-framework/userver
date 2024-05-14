#include <userver/tracing/manager.hpp>

#include <userver/engine/task/inherited_variable.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/task_inherited_request.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

constexpr std::string_view kSampledTag = "sampled";
// default value for Sampled flag is '01' as we always write spans by
// default
constexpr std::string_view kDefaultOtelTraceFlags = "01";

// The order matter for TryFillSpanBuilderFromRequest as it returns on first
// success
constexpr Format kAllFormatsOrdered[] = {
    Format::kOpenTelemetry,
    Format::kB3Alternative,
    Format::kYandexTaxi,
    Format::kYandex,
};

/// @brief Per-request data that should be available inside handlers
/// https://opentelemetry.io
struct OTelTracingHeadersInheritedData final {
  std::string tracestate;
  /// traceflags, 2 bytes
  std::string traceflags;
};

/// @see TracingHeadersInheritedData for details on the contents.
engine::TaskInheritedVariable<OTelTracingHeadersInheritedData>
    kOTelTracingHeadersInheritedData;

bool B3TryFillSpanBuilderFromRequest(const server::http::HttpRequest& request,
                                     tracing::SpanBuilder& span_builder) {
  namespace b3 = http::headers::b3;
  const auto& trace_id = request.GetHeader(b3::kTraceId);
  if (trace_id.empty()) {
    return false;
  }

  const auto& sampled = request.GetHeader(b3::kSampled);
  if (sampled.empty()) {
    return false;
  }

  span_builder.SetTraceId(trace_id);
  span_builder.SetParentSpanId(request.GetHeader(b3::kSpanId));
  span_builder.AddTagFrozen(std::string{kSampledTag}, sampled);
  return true;
}

template <class T>
void B3FillWithTracingContext(const tracing::Span& span, T& target) {
  namespace b3 = http::headers::b3;
  target.SetHeader(b3::kTraceId, span.GetTraceId());
  target.SetHeader(b3::kSpanId, span.GetSpanId());
  target.SetHeader(b3::kParentSpanId, span.GetParentId());

  const auto& sampled = server::request::GetTaskInheritedHeader(b3::kSampled);
  if (!sampled.empty()) {
    target.SetHeader(b3::kSampled, sampled);
  } else {
    target.SetHeader(b3::kSampled, "1");
  }
}

bool OpenTelemetryTryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request,
    tracing::SpanBuilder& span_builder) {
  namespace opentelemetry = http::headers::opentelemetry;
  const auto& traceparent = request.GetHeader(opentelemetry::kTraceParent);
  if (traceparent.empty()) {
    return false;
  }

  auto extraction_result =
      tracing::opentelemetry::ExtractTraceParentData(traceparent);
  if (!extraction_result.has_value()) {
    LOG_LIMITED_WARNING() << fmt::format(
        "Invalid traceparent header format ({}). Skipping Opentelemetry "
        "headers",
        extraction_result.error());
    return false;
  }

  auto data = std::move(extraction_result).value();

  span_builder.SetTraceId(std::move(data.trace_id));
  span_builder.SetParentSpanId(std::move(data.span_id));
  if (data.trace_flags.empty()) {
    data.trace_flags = std::string{kDefaultOtelTraceFlags};
  }

  const auto& tracestate = request.GetHeader(opentelemetry::kTraceState);
  kOTelTracingHeadersInheritedData.Set({
      tracestate,
      std::move(data.trace_flags),
  });
  return true;
}

template <class T>
void OpenTelemetryFillWithTracingContext(const tracing::Span& span, T& target,
                                         const logging::Level log_level) {
  const auto* data = kOTelTracingHeadersInheritedData.GetOptional();

  std::string_view traceflags = kDefaultOtelTraceFlags;
  if (data) {
    traceflags = data->traceflags;
  }
  auto traceparent_result = opentelemetry::BuildTraceParentHeader(
      span.GetTraceId(), span.GetSpanId(), traceflags);

  if (!traceparent_result.has_value()) {
    LOG_LIMITED(log_level) << fmt::format(
        "Cannot build opentelemetry traceparent header ({})",
        traceparent_result.error());
    return;
  }

  target.SetHeader(http::headers::opentelemetry::kTraceParent,
                   std::move(traceparent_result.value()));
  if (data && !data->tracestate.empty()) {
    target.SetHeader(http::headers::opentelemetry::kTraceState,
                     data->tracestate);
  }
}

bool YandexTaxiTryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request,
    tracing::SpanBuilder& span_builder) {
  const auto& trace_id = request.GetHeader(http::headers::kXYaTraceId);
  if (trace_id.empty()) {
    return false;
  }

  span_builder.SetTraceId(trace_id);

  const auto& parent_span_id = request.GetHeader(http::headers::kXYaSpanId);
  span_builder.SetParentSpanId(parent_span_id);

  const auto& parent_link = request.GetHeader(http::headers::kXYaRequestId);
  if (!parent_link.empty()) span_builder.SetParentLink(parent_link);

  return true;
}

template <class T>
void YandexTaxiFillWithTracingContext(const tracing::Span& span, T& target) {
  target.SetHeader(http::headers::kXYaRequestId, span.GetLink());
  target.SetHeader(http::headers::kXYaTraceId, span.GetTraceId());
  target.SetHeader(http::headers::kXYaSpanId, span.GetSpanId());
}

bool YandexTryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request,
    tracing::SpanBuilder& span_builder) {
  const auto& trace_id = request.GetHeader(http::headers::kXRequestId);
  if (trace_id.empty()) {
    return false;
  }

  span_builder.SetTraceId(trace_id);
  return true;
}

template <class T>
void YandexFillWithTracingContext(const tracing::Span& span, T& target) {
  target.SetHeader(http::headers::kXRequestId, span.GetTraceId());
}

}  // namespace

Format FormatFromString(std::string_view format) {
  constexpr utils::TrivialBiMap kToFormat = [](auto selector) {
    return selector()
        .Case("b3-alternative", Format::kB3Alternative)
        .Case("opentelemetry", Format::kOpenTelemetry)
        .Case("taxi", Format::kYandexTaxi)
        .Case("yandex", Format::kYandex);
  };

  auto value = kToFormat.TryFind(format);
  if (!value) {
    throw std::runtime_error(
        fmt::format("Unknown tracing format '{}' (must be one of {})", format,
                    kToFormat.DescribeFirst()));
  }
  return *value;
}

bool TryFillSpanBuilderFromRequest(Format format,
                                   const server::http::HttpRequest& request,
                                   SpanBuilder& span_builder) {
  switch (format) {
    case Format::kYandexTaxi:
      return YandexTaxiTryFillSpanBuilderFromRequest(request, span_builder);
    case Format::kYandex:
      return YandexTryFillSpanBuilderFromRequest(request, span_builder);
    case Format::kOpenTelemetry:
      return OpenTelemetryTryFillSpanBuilderFromRequest(request, span_builder);
    case Format::kB3Alternative:
      return B3TryFillSpanBuilderFromRequest(request, span_builder);
  }

  UINVARIANT(false, "Unexpected format of tracing headers");
}

void FillRequestWithTracingContext(
    Format format, const tracing::Span& span,
    clients::http::RequestTracingEditor request) {
  switch (format) {
    case Format::kYandexTaxi:
      YandexTaxiFillWithTracingContext(span, request);
      return;
    case Format::kYandex:
      YandexFillWithTracingContext(span, request);
      return;
    case Format::kOpenTelemetry:
      OpenTelemetryFillWithTracingContext(span, request,
                                          logging::Level::kWarning);
      return;
    case Format::kB3Alternative:
      B3FillWithTracingContext(span, request);
      return;
  }

  UINVARIANT(false, "Unexpected format of tracing headers");
}

void FillResponseWithTracingContext(Format format, const Span& span,
                                    server::http::HttpResponse& response) {
  switch (format) {
    case Format::kYandexTaxi:
      YandexTaxiFillWithTracingContext(span, response);
      return;
    case Format::kYandex:
      YandexFillWithTracingContext(span, response);
      return;
    case Format::kOpenTelemetry:
      // We can only fail to set otel header from Span here if the request did
      // not provide otel-compatible tracing headers. In this case the external
      // client will surely be satisfied with response tracing headers in the
      // original format. Thus we swallow the Span -> otel conversion error, if
      // any.
      OpenTelemetryFillWithTracingContext(span, response,
                                          logging::Level::kTrace);
      return;
    case Format::kB3Alternative:
      B3FillWithTracingContext(span, response);
      return;
  }

  UINVARIANT(false, "Unexpected format to send tracing headers");
}

bool GenericTracingManager::TryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request, SpanBuilder& span_builder) const {
  for (auto format : kAllFormatsOrdered) {
    if (!(in_request_response_ & format)) {
      continue;
    }

    if (tracing::TryFillSpanBuilderFromRequest(format, request, span_builder)) {
      return true;
    }
  }
  return false;
}

void GenericTracingManager::FillRequestWithTracingContext(
    const tracing::Span& span,
    clients::http::RequestTracingEditor request) const {
  for (auto format : kAllFormatsOrdered) {
    if (new_request_ & format) {
      tracing::FillRequestWithTracingContext(format, span, request);
    }
  }
}

void GenericTracingManager::FillResponseWithTracingContext(
    const Span& span, server::http::HttpResponse& response) const {
  for (auto format : kAllFormatsOrdered) {
    if (in_request_response_ & format) {
      tracing::FillResponseWithTracingContext(format, span, response);
    }
  }
}

}  // namespace tracing

USERVER_NAMESPACE_END
