#pragma once

/// @file userver/tracing/manager.hpp
/// @brief @copybrief tracing::TracingManagerBase

#include <userver/clients/http/request_tracing_editor.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {
class HttpRequest;
class HttpResponse;
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
  virtual bool TryFillSpanBuilderFromRequest(
      const server::http::HttpRequest& request,
      SpanBuilder& span_builder) const = 0;

  /// Fill request with tracing information
  virtual void FillRequestWithTracingContext(
      const Span& span, clients::http::RequestTracingEditor request) const = 0;

  /// Fill response with tracing information
  virtual void FillResponseWithTracingContext(
      const Span& span, server::http::HttpResponse& response) const = 0;
};

/// @brief Used as default tracing manager.
/// Provides methods for working with usual Yandex.Taxi tracing headers.
class DefaultTracingManager final : public TracingManagerBase {
 public:
  bool TryFillSpanBuilderFromRequest(const server::http::HttpRequest& request,
                                     SpanBuilder& span_builder) const override;

  void FillRequestWithTracingContext(
      const tracing::Span& span,
      clients::http::RequestTracingEditor request) const override;

  void FillResponseWithTracingContext(
      const Span& span, server::http::HttpResponse& response) const override;
};

extern const DefaultTracingManager kDefaultTracingManager;

}  // namespace tracing

USERVER_NAMESPACE_END
