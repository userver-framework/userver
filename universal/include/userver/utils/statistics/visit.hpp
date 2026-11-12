#pragma once

/// @file userver/utils/statistics/visit.hpp
/// @brief Visit Writer-based metrics through a custom format builder.
/// @ingroup userver_universal

#include <userver/utils/function_ref.hpp>
#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Non-owning reference to a function that writes metrics via @ref utils::statistics::Writer.
using WriterFuncRef = utils::function_ref<void(Writer&) const>;

/// @brief Low-level connecting function that dumps metrics from @a func into @a out.
///
/// Calls @a func with a @ref utils::statistics::Writer. Each metric written through that `Writer`
/// is forwarded to `out.HandleMetric`. @a request filters the metrics and may
/// attach extra labels; `request.prefix` is a match filter, not a path to write
/// under.
///
/// This function dumps Writer-based metrics. It is used to implement formats such as:
/// - @ref utils::statistics::ToPrometheusFormat "Prometheus"
/// - @ref utils::statistics::ToGraphiteFormat "Graphite"
/// - @ref utils::statistics::ToJsonFormat "JSON"
/// - @ref utils::statistics::ToPrettyFormat "pretty format"
/// - @ref utils::statistics::ToSolomonFormat "Solomon"
/// - @ref utils::statistics::GetPortabilityWarnings "portability info"
///
/// To define a custom output format, implement @ref utils::statistics::BaseFormatBuilder:
/// @snippet universal/src/utils/statistics/visit_test.cpp  VisitMetrics custom format
///
/// Collect metrics using the custom format builder:
/// @snippet universal/src/utils/statistics/visit_test.cpp  VisitMetrics custom format usage
///
/// @param func writes metrics to the provided @ref utils::statistics::Writer
/// @param out receives each written metric via @ref utils::statistics::BaseFormatBuilder::HandleMetric
/// @param request metric filter and extra labels
void VisitMetrics(WriterFuncRef func, BaseFormatBuilder& out, const Request& request = {});

/// @overload
///
/// Dumps @a metric via `writer = metric` (`DumpMetric` / built-in Writer support).
void VisitMetrics(const HasWriterSupport auto& metric, BaseFormatBuilder& out, const Request& request = {}) {
    utils::statistics::VisitMetrics([&metric](Writer& writer) { writer = metric; }, out, request);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
