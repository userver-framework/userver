#pragma once

#include <string>

#include <userver/utils/expected.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing::opentelemetry {

inline constexpr std::size_t kTraceIdSize = 32;
inline constexpr std::size_t kSpanIdSize = 16;

struct TraceParentDataView {
    std::string_view version;
    std::string_view trace_id;
    std::string_view span_id;
    std::string_view trace_flags;
};

utils::expected<TraceParentDataView, USERVER_NAMESPACE::utils::StringLiteral> ExtractTraceParentDataView(
    std::string_view trace_parent
) noexcept;

utils::expected<std::string, std::string> BuildTraceParentHeader(
    std::string_view trace_id,
    std::string_view span_id,
    std::string_view trace_flags
);

}  // namespace tracing::opentelemetry

USERVER_NAMESPACE_END
