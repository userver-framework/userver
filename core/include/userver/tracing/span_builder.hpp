#pragma once

/// @file userver/tracing/span_builder.hpp
/// @brief @copybrief tracing::SpanBuilder

#include <string>

#include <userver/tracing/span.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Provides interface for editing Span, before final building.
class SpanBuilder final {
 public:
  explicit SpanBuilder(std::string name,
                       const utils::impl::SourceLocation& location =
                           utils::impl::SourceLocation::Current());

  void SetTraceId(std::string trace_id);
  const std::string& GetTraceId() const noexcept;
  void SetSpanId(std::string span_id);
  void SetParentSpanId(std::string parent_span_id);
  void SetParentLink(std::string parent_link);
  void AddTagFrozen(std::string key, logging::LogExtra::Value value);
  Span Build() &&;

 private:
  std::unique_ptr<Span::Impl, Span::OptionalDeleter> pimpl_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
