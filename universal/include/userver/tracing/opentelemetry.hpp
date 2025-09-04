#pragma once

#include <string>

#include <userver/utils/expected.hpp>
#include <userver/utils/small_string.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing::opentelemetry {

inline constexpr std::size_t kTraceIdSize = 32;
inline constexpr std::size_t kSpanIdSize = 16;

struct TraceParentData {
    std::string version;
    utils::SmallString<kTraceIdSize> trace_id;
    utils::SmallString<kSpanIdSize> span_id;
    std::string trace_flags;
};

utils::expected<TraceParentData, std::string> ExtractTraceParentData(std::string_view trace_parent);

utils::expected<std::string, std::string>
BuildTraceParentHeader(std::string_view trace_id, std::string_view span_id, std::string_view trace_flags);

}  // namespace tracing::opentelemetry

USERVER_NAMESPACE_END
