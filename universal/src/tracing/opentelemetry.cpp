#include <userver/tracing/opentelemetry.hpp>

#include <vector>

#include <fmt/format.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing::opentelemetry {

namespace {

constexpr std::size_t kTraceParentSize = 55;
constexpr std::size_t kVersionSize = 2;
constexpr std::size_t kTraceFlagsSize = 2;

constexpr char kDefaultVersion[] = "00";

}  // namespace

utils::expected<TraceParentDataView, USERVER_NAMESPACE::utils::StringLiteral> ExtractTraceParentDataView(
    std::string_view trace_parent
) noexcept {
    // https://uptrace.dev/get/opentelemetry-dotnet/traceparent
    // trace_parent has the format "00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-01"
    if (trace_parent.size() != kTraceParentSize) {
        return utils::unexpected(utils::StringLiteral{"Invalid OpenTelemetry header size"});
    }

    TraceParentDataView result;

    result.version = trace_parent.substr(0, kVersionSize);
    if (result.version != "00" && result.version != "01") {
        return utils::unexpected(utils::StringLiteral{"Invalid version in the OpenTelemetry header"});
    }
    trace_parent.remove_prefix(kVersionSize);

    using MemberPointerType = std::string_view TraceParentDataView::*;
    static constexpr std::pair<std::size_t, MemberPointerType> kChunkSizesAndMemberPtrsAfterVersion[] = {
        {kTraceIdSize, &TraceParentDataView::trace_id},
        {kSpanIdSize, &TraceParentDataView::span_id},
        {kTraceFlagsSize, &TraceParentDataView::trace_flags},
    };
    for (const auto& [pos, field] : kChunkSizesAndMemberPtrsAfterVersion) {
        UASSERT_MSG(pos < trace_parent.size(), "Initial size check or logic is incorrect");

        if (trace_parent[0] != '-') {
            return utils::unexpected(utils::StringLiteral{"One of the OpenTelemetry header fields has invalid size"});
        }

        const auto unchecked_field = trace_parent.substr(1, pos);
        trace_parent.remove_prefix(pos + 1);

        if (!utils::encoding::IsHexData(unchecked_field)) {
            return utils::unexpected(
                unchecked_field.find('-') == std::string_view::npos
                    ? utils::StringLiteral{"One of the OpenTelemetry header fields is not hex data"}
                    : utils::StringLiteral{"One of the OpenTelemetry header fields has invalid size"}
            );
        }

        result.*field = unchecked_field;
    }
    UASSERT_MSG(trace_parent.empty(), "Initial size check or logic is incorrect");

    return result;
}

utils::expected<std::string, std::string> BuildTraceParentHeader(
    std::string_view trace_id,
    std::string_view span_id,
    std::string_view trace_flags
) {
    if (trace_id.size() != kTraceIdSize) {
        return utils::unexpected("Invalid trace_id size");
    }
    if (!utils::encoding::IsHexData(trace_id)) {
        return utils::unexpected(fmt::format("Invalid trace_id value: '{}' is not a hex", trace_id));
    }

    if (span_id.size() != kSpanIdSize) {
        return utils::unexpected("Invalid span_id size");
    }
    if (!utils::encoding::IsHexData(span_id)) {
        return utils::unexpected(fmt::format("Invalid span_id value: '{}' is not a hex", span_id));
    }

    if (trace_flags.size() != kTraceFlagsSize) {
        return utils::unexpected("Invalid trace_flags size");
    }
    if (!utils::encoding::IsHexData(trace_flags)) {
        return utils::unexpected(fmt::format("Invalid trace_flags value: '{}' is not a hex", trace_flags));
    }

    return utils::StrCat(kDefaultVersion, "-", trace_id, "-", span_id, "-", trace_flags);
}

}  // namespace tracing::opentelemetry

USERVER_NAMESPACE_END
