#pragma once

/// @file userver/tracing/span_builder.hpp
/// @brief @copybrief tracing::SpanBuilder

#include <string>
#include <string_view>

#include <userver/tracing/span.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Provides interface for editing Span, before final building.
class SpanBuilder final {
public:
    explicit SpanBuilder(
        std::string name,
        const utils::impl::SourceLocation& location = utils::impl::SourceLocation::Current()
    );

    void SetTraceId(std::string_view trace_id);
    std::string_view GetTraceId() const noexcept;
    void SetSpanId(std::string_view span_id);
    void SetLink(std::string_view link);
    void SetParentSpanId(std::string_view parent_span_id);
    void SetParentLink(std::string_view parent_link);

    void AddTagFrozen(std::string key, logging::LogExtra::Value value);
    void AddNonInheritableTag(std::string key, logging::LogExtra::Value value);

    Span Build() &&;
    Span BuildDetachedFromCoroStack() &&;

private:
    std::unique_ptr<Span::Impl, Span::OptionalDeleter> pimpl_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
