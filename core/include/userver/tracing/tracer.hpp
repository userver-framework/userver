#pragma once

/// @file userver/tracing/tracer.hpp
/// @brief Tracing configuration for span logging suppression

#include <string>

#include <dynamic_config/variables/USERVER_NO_LOG_SPANS.hpp>

USERVER_NAMESPACE_BEGIN

/// Tracing support via @ref tracing::Span
namespace tracing {

using NoLogSpans = ::dynamic_config::userver_no_log_spans::VariableType;

/// Sets the global configuration for disabling logging some of the @ref tracing::Span.
void SetNoLogSpans(NoLogSpans&& spans);

/// Returns true iff the @ref tracing::Span with `name` is not logged.
bool IsNoLogSpan(const std::string& name);

}  // namespace tracing

USERVER_NAMESPACE_END
