#pragma once

#include <string>

#include <userver/utils/expected.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing::opentelemetry {

struct TraceParentData {
  std::string version;
  std::string trace_id;
  std::string span_id;
  std::string trace_flags;
};

utils::expected<TraceParentData, std::string> ExtractTraceParentData(
    std::string_view trace_parent);

utils::expected<std::string, std::string> BuildTraceParentHeader(
    std::string_view trace_id, std::string_view span_id,
    std::string_view trace_flags);

}  // namespace tracing::opentelemetry

USERVER_NAMESPACE_END
