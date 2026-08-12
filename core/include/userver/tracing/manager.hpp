#pragma once

/// @file userver/tracing/manager.hpp
/// @brief @copybrief tracing::TracingManagerBase

#include <userver/clients/http/middlewares/base.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {
class HttpResponse;
class HttpRequest;
}  // namespace server::http

namespace tracing {

/// @ingroup userver_base_classes
///
/// @brief Base class for propagating trace context information in headers.
///
/// Mostly used by tracing::DefaultTracingManagerLocator.
class TracingManagerBase {
public:
    virtual ~TracingManagerBase() = default;

    /// Fill SpanBuilder params with actual tracing information extracted from the
    /// request. You should build Span with SpanBuilder::Build, after calling
    /// this.
    /// @return Returns bool, that tells us was any of tracing headers used to
    /// create new span
    virtual bool TryFillSpanBuilderFromRequest(const server::http::HttpRequest& request, SpanBuilder& span_builder)
        const = 0;

    /// Fill new client requests with tracing information
    virtual void FillRequestWithTracingContext(const Span& span, clients::http::MiddlewareRequest request) const = 0;

    /// Fill response with tracing information
    virtual void FillResponseWithTracingContext(const Span& span, server::http::HttpResponse& response) const = 0;
};

// clang-format off
enum class Format : short {
  /// Yandex Taxi/Lavka/Eda/... tracing:
  /// @code
  /// http::headers::kXYaTraceId -> tracing::Span::GetTraceId() -> http::headers::kXYaTraceId
  /// http::headers::kXYaRequestId -> tracing::Span::GetParentLink(); tracing::Span::GetLink() -> http::headers::kXYaRequestId
  /// http::headers::kXYaSpanId -> tracing::Span::GetParentId(); tracing::Span::GetSpanId() -> http::headers::kXYaSpanId
  /// @endcode
  kYandexTaxi = 1 << 1,

  /// Yandex Search tracing:
  /// http::headers::kXRequestId -> tracing::Span::GetTraceId() -> http::headers::kXRequestId
  kYandex = 1 << 2,

  /// Use http::headers::opentelemetry::kTraceState and
  /// http::headers::opentelemetry::kTraceParent headers to fill the
  /// tracing::opentelemetry::TraceParentData as per OpenTelemetry.
  kOpenTelemetry = 1 << 3,

  /// Openzipkin b3 alternative propagation, where Span ID goes to parent ID:
  /// @code
  /// b3::kTraceId -> tracing::Span::GetTraceId() -> b3::kTraceId
  /// b3::kSpanId -> tracing::Span::GetParentId(); tracing::Span::GetSpanId() -> b3::kSpanId
  /// span.GetParentId() -> b3::kParentSpanId
  /// @endcode
  /// See https://github.com/openzipkin/b3-propagation for more info.
  kB3Alternative = 1 << 4,
};
// clang-format on

/// Converts a textual representation of format into tracing::Format enum.
Format FormatFromString(std::string_view format);

bool TryFillSpanBuilderFromRequest(Format format, const server::http::HttpRequest& request, SpanBuilder& span_builder);

void FillRequestWithTracingContext(Format format, const tracing::Span& span, clients::http::MiddlewareRequest request);

void FillResponseWithTracingContext(Format format, const Span& span, server::http::HttpResponse& response);

/// W3C trace-flags byte values from the @c traceparent header.
/// @see https://www.w3.org/TR/trace-context/#trace-flags
enum OtelTraceFlags : std::uint8_t {
    kNoTracing = 0x00,
    kSampled = 0x01,
};

/// @returns the W3C trace-flags byte from the incoming request, stored in
/// @ref engine::TaskInheritedVariable. Defaults to @ref OtelTraceFlags::kSampled
/// when absent.
OtelTraceFlags GetInheritedOtelTraceFlags();

/// @returns the W3C `tracestate` header value from the incoming request, stored
/// in @ref engine::TaskInheritedVariable. Empty string_view when absent.
std::string_view GetInheritedOtelTraceState();

/// Stores `tracestate` and the parsed `traceflags` hex byte into
/// @ref engine::TaskInheritedVariable so they are available for outgoing
/// request propagation within the current task tree.
void SetInheritedOtelTracingData(std::string_view tracestate, std::string_view traceflags);

/// @brief Generic tracing manager that knows about popular tracing
/// headers and allows customising input and output headers.
class GenericTracingManager final : public TracingManagerBase {
public:
    enum class SamplingEnabled : bool {
        kNo = false,
        kYes = true,
    };

    GenericTracingManager() = delete;

    GenericTracingManager(
        utils::Flags<Format> in_request_response,
        utils::Flags<Format> new_request,
        SamplingEnabled sampling = SamplingEnabled::kNo
    )
        : in_request_response_{in_request_response},
          new_request_{new_request},
          sampling_{sampling}
    {}

    bool TryFillSpanBuilderFromRequest(const server::http::HttpRequest& request, SpanBuilder& span_builder)
        const override;

    void FillRequestWithTracingContext(const tracing::Span& span, clients::http::MiddlewareRequest request)
        const override;

    void FillResponseWithTracingContext(const Span& span, server::http::HttpResponse& response) const override;

private:
    const utils::Flags<Format> in_request_response_;
    const utils::Flags<Format> new_request_;
    const SamplingEnabled sampling_{SamplingEnabled::kNo};
};

}  // namespace tracing

USERVER_NAMESPACE_END
