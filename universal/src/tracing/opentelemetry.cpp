#include <userver/tracing/opentelemetry.hpp>

#include <vector>

#include <fmt/format.h>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing::opentelemetry {

namespace {

constexpr std::size_t kTraceParentSize = 55;
constexpr std::size_t kVersionSize = 2;
constexpr std::size_t kTraceIdSize = 32;
constexpr std::size_t kSpanIdSize = 16;
constexpr std::size_t kTraceFlagsSize = 2;

constexpr char kDefaultVersion[] = "00";

}  // namespace

utils::expected<TraceParentData, std::string> ExtractTraceParentData(
    std::string_view trace_parent) {
  if (trace_parent.size() != kTraceParentSize) {
    return utils::unexpected("Invalid header size");
  }

  std::vector<std::string_view> trace_fields =
      utils::text::SplitIntoStringViewVector(trace_parent, "-");

  if (trace_fields.size() != 4) {
    return utils::unexpected("Invalid fields count");
  }

  for (const auto& field : trace_fields) {
    if (!utils::encoding::IsHexData(field)) {
      return utils::unexpected("One of the fields is not hex data");
    }
  }

  const std::string_view version{trace_fields[0]};
  const std::string_view trace_id{trace_fields[1]};
  const std::string_view span_id{trace_fields[2]};
  const std::string_view trace_flags{trace_fields[3]};

  if (version.size() != kVersionSize || trace_id.size() != kTraceIdSize ||
      span_id.size() != kSpanIdSize || trace_flags.size() != kTraceFlagsSize) {
    return utils::unexpected("One of the fields has invalid size");
  }

  return TraceParentData{
      std::string{version},
      std::string{trace_id},
      std::string{span_id},
      std::string{trace_flags},
  };
}

utils::expected<std::string, std::string> BuildTraceParentHeader(
    std::string_view trace_id, std::string_view span_id,
    std::string_view trace_flags) {
  if (trace_id.size() != kTraceIdSize) {
    return utils::unexpected("Invalid trace_id size");
  }
  if (!utils::encoding::IsHexData(trace_id)) {
    return utils::unexpected(
        fmt::format("Invalid trace_id value: '{}' is not a hex", trace_id));
  }

  if (span_id.size() != kSpanIdSize) {
    return utils::unexpected("Invalid span_id size");
  }
  if (!utils::encoding::IsHexData(span_id)) {
    return utils::unexpected(
        fmt::format("Invalid span_id value: '{}' is not a hex", span_id));
  }

  if (trace_flags.size() != kTraceFlagsSize) {
    return utils::unexpected("Invalid trace_flags size");
  }
  if (!utils::encoding::IsHexData(trace_flags)) {
    return utils::unexpected(fmt::format(
        "Invalid trace_flags value: '{}' is not a hex", trace_flags));
  }

  return fmt::format("{}-{}-{}-{}", kDefaultVersion, trace_id, span_id,
                     trace_flags);
}

}  // namespace tracing::opentelemetry

USERVER_NAMESPACE_END
